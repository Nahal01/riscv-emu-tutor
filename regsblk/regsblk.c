//----------------------------------------------------------------------------------------------
//
// Author   : Nahal Fadaei
// Function : A simple device driver with a few scratchpad registers mapped into host memory
//            The number of registers are obtained from device treee
//
//----------------------------------------------------------------------------------------------

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>

#define DEVICE_NAME "regsblk"

struct regsblk_dev {
    u32 *registers;
    u32 num_registers;
    struct cdev cdev;
    dev_t devt;
    struct class *class;
};

static struct regsblk_dev regsblk;

static int regsblk_open(struct inode *inode, struct file *file)
{
    file->private_data = &regsblk;
    return 0;
}

static ssize_t regsblk_read(struct file *file, char __user *buf, size_t len, loff_t *ppos)
{
    struct regsblk_dev *dev = file->private_data;
    u32 value;
    size_t index = *ppos / sizeof(u32);

    if (index >= dev->num_registers)
        return -EINVAL;

    value = dev->registers[index];

    if (copy_to_user(buf, &value, sizeof(u32)))
        return -EFAULT;

    *ppos += sizeof(u32);
    return sizeof(u32);
}

static ssize_t regsblk_write(struct file *file, const char __user *buf, size_t len, loff_t *ppos)
{
    struct regsblk_dev *dev = file->private_data;
    u32 value;
    size_t index = *ppos / sizeof(u32);

    if (index >= dev->num_registers)
        return -EINVAL;

    if (copy_from_user(&value, buf, sizeof(u32)))
        return -EFAULT;

    dev->registers[index] = value;

    *ppos += sizeof(u32);
    return sizeof(u32);
}

static loff_t regsblk_llseek(struct file *file, loff_t offset, int whence)
{
    loff_t new_pos;

    switch (whence) {
    case SEEK_SET:
        new_pos = offset;
        break;
    case SEEK_CUR:
        new_pos = file->f_pos + offset;
        break;
    case SEEK_END:
        new_pos = regsblk.num_registers * sizeof(u32) + offset;
        break;
    default:
        return -EINVAL;
    }

    if (new_pos < 0 || new_pos > regsblk.num_registers * sizeof(u32))
        return -EINVAL;

    file->f_pos = new_pos;
    return new_pos;
}

static const struct file_operations regsblk_fops = {
    .owner = THIS_MODULE,
    .open = regsblk_open,
    .read = regsblk_read,
    .write = regsblk_write,
    .llseek = regsblk_llseek,
};

static int regsblk_probe(struct platform_device *pdev)
{
    struct resource *res;
    struct device *dev = &pdev->dev;
    int ret;

    //reading property from DTB
    if (of_property_read_u32(dev->of_node, "num-registers", &regsblk.num_registers)) {
        dev_err(dev, "num-registers not found in device tree\n");
        return -EINVAL;
    }

    //allocate dummy registers space
    regsblk.registers = devm_kzalloc(dev, regsblk.num_registers * sizeof(u32), GFP_KERNEL);
    if (!regsblk.registers)
        return -ENOMEM;

    ret = alloc_chrdev_region(&regsblk.devt, 0, 1, DEVICE_NAME);
    if (ret)
        return ret;

    cdev_init(&regsblk.cdev, &regsblk_fops);
    regsblk.cdev.owner = THIS_MODULE;
    ret = cdev_add(&regsblk.cdev, regsblk.devt, 1);
    if (ret)
        goto unregister_chrdev;

      //regsblk.class = class_create(THIS_MODULE, DEVICE_NAME); // older linux kernel (<6.11)
      regsblk.class = class_create(DEVICE_NAME);

    if (IS_ERR(regsblk.class)) {
        ret = PTR_ERR(regsblk.class);
        goto del_cdev;
    }

    device_create(regsblk.class, NULL, regsblk.devt, NULL, DEVICE_NAME);
    dev_info(dev, "regsblk device initialized\n");
    return 0;

del_cdev:
    cdev_del(&regsblk.cdev);
unregister_chrdev:
    unregister_chrdev_region(regsblk.devt, 1);
    return ret;
}

static void regsblk_remove(struct platform_device *pdev)
{
    device_destroy(regsblk.class, regsblk.devt);
    class_destroy(regsblk.class);
    cdev_del(&regsblk.cdev);
    unregister_chrdev_region(regsblk.devt, 1);
}

static const struct of_device_id regsblk_of_match[] = {
    { .compatible = "myfwco,regsblk" },
    { }
};
MODULE_DEVICE_TABLE(of, regsblk_of_match);

static struct platform_driver regsblk_driver = {
    .probe  = regsblk_probe,
    .remove = regsblk_remove,
    .driver = {
        .name = DEVICE_NAME,
        .of_match_table = regsblk_of_match,
    },
};

module_platform_driver(regsblk_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nahal Fadaei");
MODULE_DESCRIPTION("RegsBlk Character Device Driver");

