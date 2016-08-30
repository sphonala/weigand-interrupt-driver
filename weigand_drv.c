#include <linux/kernel.h>  
#include <linux/module.h>  
#include <linux/cdev.h>  
#include <linux/fs.h>  
#include <linux/device.h>  
#include <linux/syscalls.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/uaccess.h>  
#include <linux/string.h> 
 #include <linux/ktime.h>
#include "weigand-drv.h"
/* ---- Public Variables ------------------------------------------------- */


/* ---- Private Constants and Types -------------------------------------- */
	
static char gBanner[] __initdata = KERN_INFO "User Mode Weigand(Interrupt) Driver: 1\n";

/* ---- Private Variables ------------------------------------------------ */
	static volatile int var=0,cardno=0,p1=0,p0=0,temp=1;
	static int irq_ret=0,irq=0,irq2=0,gpio1=0,gpio2;
	//static unsigned long flags;
	ktime_t ktime_get(void);
static ktime_t start, end;
//static s64 t1;
	#define GPIO_DRV_DEV_NAME "weigand"
	static dev_t gGpioDrvDevNum = 0;
	static struct class *gGpioDrvClass = NULL;
	static struct cdev gGpioDrvCDev;
	struct fasync_struct *async_queue;

/* ---- Private Function Prototypes -------------------------------------- */


/* ---- Functions ------------------------------------------------------- */

irqreturn_t irq_handler(int irqno, void *dev_id)
{
//local_irq_disable();
//printk("driver1 irq handler var==>%d \n",var);
end=ktime_get();
if(ktime_to_ns(ktime_sub(end, start))>2050000)
var=0;

	if(irqno==irq)
	{
	cardno<<=1;
	cardno|=0x01;var++;
	}
	else{ cardno<<=1;var++;}
  	
	if(var==26)
	{
		for(var=1;var<=26;var++)
                        {
                                if((cardno&temp)==temp)
                                {
                                  if(var<14)p1++;
                                  if(var>13)p0++;
                                }
                        temp<<=1;
                        }
                         //printk("In driver2 Interrupt handler var=%d cardno=%x p1=%d p0=%d\n", var,cardno,p1,p0);
                        p0%=2;p1%=2;
                        if(p0==0&&p1==1)
                        {

			cardno>>=1;
			cardno&=0xFFFFFF;
			printk("In driver1 cardno=%x**\n",cardno);
			irq_ret=cardno;
			kill_fasync(&async_queue, SIGIO, POLL_IN);
			}var=0;cardno=0;temp=1;p0=0;p1=0;
	}
	
	start=ktime_get();
	//local_irq_enable();
	return IRQ_HANDLED;

}


static ssize_t weigand_read(struct file *fd, char __user *buf, size_t len, loff_t *ptr)  
{  
    
	if(copy_to_user(buf, &irq_ret, sizeof(int)))  
        return -EFAULT;  

    	return 0;  
}  
static int weigand_fasync(int fd, struct file *filp, int mode)
{
    	fasync_helper(fd, filp, mode, &async_queue);
    	return 0;
}

/*static int fpga_key_release(struct inode *node, struct file *fd)  
{  
 
    weigand_fasync(-1, fd, 0);
    
    return 0;  
}*/
 

/****************************************************************************
*
* Called when the GPIO driver ioctl is made
*
***************************************************************************/
long gpio_drv_ioctl( struct file *file, unsigned int cmd, unsigned long arg )
{
	int rc = 0;
	switch ( cmd )
	{
	case GPIO_IOCTL_REQUEST:
	{
		GPIO_Request_t request;
		char *label;
		int labelLen;
		if ( copy_from_user( &request, (GPIO_Request_t *)arg, sizeof( request )) != 0 )
		{
		return -EFAULT;
		}
		/*
		* gpiolib stores a pointer to the string that we pass in, and doesn't give us
		* any way to release it. So we introduce a small memory leak, and allocate a
		* some memory to put a copy of the label into. It's not anticipated that many
		* actual usermode programs will use this, and it will be a very low frequency
		* thing, so this should be ok.
		*/
		
		request.label[ sizeof( request.label ) - 1 ] = '\0';
		labelLen = strlen( request.label ) + 5;
		label = kmalloc( labelLen, GFP_KERNEL );
		strlcpy( label, "UM: ", labelLen );
		strlcat( label, request.label, labelLen );
		rc = gpio_request( request.gpio, label );
		break;
	}
	case GPIO_IOCTL_FREE:
	{
		gpio_free( (unsigned)arg );
		break;
	}
	case GPIO_IOCTL_DIRECTION_INPUT:
	{
		rc = gpio_direction_input( (unsigned)arg );
		break;
	}
	
	case GPIO_IOCTL_DIRECTION_OUTPUT:
	{
		GPIO_Value_t param;
		if ( copy_from_user( &param, (GPIO_Value_t *)arg, sizeof( param )) != 0 )
		{
		return -EFAULT;
		}
		rc = gpio_direction_output( param.gpio, param.value );
		break;
	}

	case GPIO_IOCTL_GET_VALUE:
	{
		GPIO_Value_t param;
		if ( copy_from_user( &param, (GPIO_Value_t *)arg, sizeof( param )) != 0 )
		{
		return -EFAULT;
		}
		param.value = gpio_get_value( param.gpio );
		if ( copy_to_user( (GPIO_Value_t *)arg, &param, sizeof( param )) != 0 )
		{
		return -EFAULT;
		}
		break;
	}

	case GPIO_IOCTL_SET_VALUE:
	{
		GPIO_Value_t param;
		if ( copy_from_user( &param, (GPIO_Value_t *)arg, sizeof( param )) != 0 )
		{
		return -EFAULT;
		}
		gpio_set_value( param.gpio, param.value );
		break;
	}

	case GPIO_IOCTL_ISR:
	{
		GPIO_Request_t request;
		char *label;
		int labelLen;
		if ( copy_from_user( &request, (GPIO_Request_t *)arg, sizeof( request )) != 0 )
		{
		return -EFAULT;
		}
		/*
		* gpiolib stores a pointer to the string that we pass in, and doesn't give us
		* any way to release it. So we introduce a small memory leak, and allocate a
		* some memory to put a copy of the label into. It's not anticipated that many
		* actual usermode programs will use this, and it will be a very low frequency
		* thing, so this should be ok.
		*/
		
		request.label[ sizeof( request.label ) - 1 ] = '\0';
		labelLen = strlen( request.label ) + 6;
		label = kmalloc( labelLen, GFP_KERNEL );
		strlcpy( label, "UM1: ", labelLen );
		strlcat( label, request.label, labelLen );
		if(gpio1==0)
		gpio1=request.gpio;
		else
		{gpio2=request.gpio;}
		irq= gpio_to_irq( request.gpio );
		printk(KERN_INFO"irq=%d\n",irq);
		if(irq2==0)
		irq2=irq;	
		if(irq < 0){
		gpio_free(request.gpio);
	        printk(KERN_ERR "gpio_to_irq() failed !\n");}

		#if 0
		   rc = request_irq(irq,
                    (irq_handler_t)irq_handler,
                    IRQF_TRIGGER_FALLING | IRQF_SHARED,
                    label,
                    &gGpioDrvDevNum);
		#else
		   rc = request_irq(irq,
                    (irq_handler_t)irq_handler,
                    IRQF_TRIGGER_FALLING | IRQF_DISABLED,
                    label,
                    &gGpioDrvDevNum);
		#endif
 		
		if(rc)
		{
		 gpio_free(request.gpio);
	         printk(KERN_ERR "request_irq() failed ! %d\n", rc);}
		 break;
		}

	default:
	{
		printk( KERN_WARNING "%s: Unrecognized ioctl: '0x%x'\n", __func__, cmd );
		return -ENOTTY;
	}
	}
return rc;
}

/****************************************************************************
*
* File Operations (these are the device driver entry points)
*
***************************************************************************/
struct file_operations gpio_fops =
{
.owner= THIS_MODULE,
.fasync = weigand_fasync,
//.release = fpga_key_release,  
.read= weigand_read,
.unlocked_ioctl= gpio_drv_ioctl,
};


/****************************************************************************
*
* Called to perform module initialization when the module is loaded
*
***************************************************************************/
static int __init weigand_drv_init( void )
{
/*start=ktime_get();
printk("*\n");
end=ktime_get();
if(((ktime_to_ns(ktime_sub(end, start))))>50000)
printk("t1=%lld\n",(long long)ktime_to_ns(end));	
else 
printk("else\n");*/
	int rc = 0;
	printk( gBanner );
	if ( MAJOR( gGpioDrvDevNum ) == 0 )
	{
	/* Allocate a major number dynaically */
	if (( rc = alloc_chrdev_region( &gGpioDrvDevNum, 0, 1, GPIO_DRV_DEV_NAME )) < 0 )
	{
	printk( KERN_WARNING "%s: alloc_chrdev_region failed; err: %d\n", __func__, rc );
	return rc;
	}
	}
	else
	{
	/* Use the statically assigned major number */
	if (( rc = register_chrdev_region( gGpioDrvDevNum, 1, GPIO_DRV_DEV_NAME )) < 0 )
	{
	printk( KERN_WARNING "%s: register_chrdev failed; err: %d\n", __func__, rc );
	return rc;
	}
	}
	cdev_init( &gGpioDrvCDev, &gpio_fops );
	gGpioDrvCDev.owner = THIS_MODULE;
	if (( rc = cdev_add( &gGpioDrvCDev, gGpioDrvDevNum, 1 )) != 0 )
	{
	printk( KERN_WARNING "%s: cdev_add failed: %d\n", __func__, rc );
	goto out_unregister;
	}
	/* Now that we've added the device, create a class, so that udev will make the /dev entry */
	gGpioDrvClass = class_create( THIS_MODULE, GPIO_DRV_DEV_NAME );
	if ( IS_ERR( gGpioDrvClass ))
	{
	printk( KERN_WARNING "%s: Unable to create class\n", __func__ );
	rc = -1;
	goto out_cdev_del;
	}
	device_create( gGpioDrvClass, NULL, gGpioDrvDevNum, NULL, GPIO_DRV_DEV_NAME );
	goto done;
	out_cdev_del:
	cdev_del( &gGpioDrvCDev );
	out_unregister:
	unregister_chrdev_region( gGpioDrvDevNum, 1 );
	done:
	return rc;
	}
	

/****************************************************************************
*
* Called to perform module cleanup when the module is unloaded.
*
***************************************************************************/

	static void __exit weigand_drv_exit( void )
	{
	gpio_free(gpio1);
	gpio_free(gpio2);
	free_irq(irq, &gGpioDrvDevNum);
	free_irq(irq2, &gGpioDrvDevNum);
	device_destroy( gGpioDrvClass, gGpioDrvDevNum );
	class_destroy( gGpioDrvClass );
	cdev_del( &gGpioDrvCDev );
	unregister_chrdev_region( gGpioDrvDevNum, 1 );
	}

/****************************************************************************/
module_init( weigand_drv_init );
module_exit( weigand_drv_exit );
MODULE_AUTHOR("Sandip Patil(sphonala@gmail.com)");
MODULE_DESCRIPTION( "User Mode Weigand(Interrupt) Driver" );
MODULE_LICENSE( "GPL" );
MODULE_VERSION( "1.0" );

