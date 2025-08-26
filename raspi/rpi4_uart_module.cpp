#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

#define DEVICE_NAME "rpi4_uart"
#define CLASS_NAME "rpi4uart"
#define BUFFER_SIZE 1024

// BCM2711 (RPi4) UART0 (PL011) base address
#define UART0_BASE 0xFE201000
#define UART_SIZE  0x1000

// UART0 Register offsets
#define UART_DR     0x00  // Data Register
#define UART_RSR    0x04  // Receive Status Register
#define UART_FR     0x18  // Flag Register
#define UART_ILPR   0x20  // IrDA Low-Power Counter Register
#define UART_IBRD   0x24  // Integer Baud Rate Divisor
#define UART_FBRD   0x28  // Fractional Baud Rate Divisor
#define UART_LCRH   0x2C  // Line Control Register
#define UART_CR     0x30  // Control Register
#define UART_IFLS   0x34  // Interrupt FIFO Level Select
#define UART_IMSC   0x38  // Interrupt Mask Set/Clear
#define UART_RIS    0x3C  // Raw Interrupt Status
#define UART_MIS    0x40  // Masked Interrupt Status
#define UART_ICR    0x44  // Interrupt Clear Register

// Flag Register bits
#define UART_FR_TXFE (1 << 7)  // Transmit FIFO empty
#define UART_FR_RXFF (1 << 6)  // Receive FIFO full
#define UART_FR_TXFF (1 << 5)  // Transmit FIFO full
#define UART_FR_RXFE (1 << 4)  // Receive FIFO empty
#define UART_FR_BUSY (1 << 3)  // UART busy

// Control Register bits
#define UART_CR_UARTEN (1 << 0)  // UART enable
#define UART_CR_TXE    (1 << 8)  // Transmit enable
#define UART_CR_RXE    (1 << 9)  // Receive enable

// Line Control Register bits
#define UART_LCRH_WLEN_8 (3 << 5)  // 8-bit word length
#define UART_LCRH_FEN    (1 << 4)  // Enable FIFOs

// Interrupt bits
#define UART_INT_RX (1 << 4)  // Receive interrupt
#define UART_INT_TX (1 << 5)  // Transmit interrupt

// GPIO pins for UART0
#define GPIO_UART_TXD 14
#define GPIO_UART_RXD 15

// Module parameters
static int major_number = 0;
static struct class* uart_class = NULL;
static struct device* uart_device = NULL;
static struct cdev uart_cdev;
static dev_t dev_number;

// Hardware resources
static void __iomem *uart_base = NULL;
static int uart_irq = 29; // UART0 IRQ on RPi4

// Buffers
static char rx_buffer[BUFFER_SIZE];
static int rx_head = 0, rx_tail = 0;
static char tx_buffer[BUFFER_SIZE];
static int tx_head = 0, tx_tail = 0;

// Synchronization
static DEFINE_SPINLOCK(uart_lock);
static DECLARE_WAIT_QUEUE_HEAD(rx_wait);
static DECLARE_WAIT_QUEUE_HEAD(tx_wait);

// Function prototypes
static int uart_open(struct inode *inode, struct file *file);
static int uart_release(struct inode *inode, struct file *file);
static ssize_t uart_read(struct file *file, char __user *buffer, size_t len, loff_t *pos);
static ssize_t uart_write(struct file *file, const char __user *buffer, size_t len, loff_t *pos);
static irqreturn_t uart_interrupt(int irq, void *dev_id);
static int uart_hw_init(void);
static void uart_hw_cleanup(void);

// File operations structure
static struct file_operations uart_fops = {
    .open = uart_open,
    .release = uart_release,
    .read = uart_read,
    .write = uart_write,
    .owner = THIS_MODULE,
};

// Helper functions for register access
static inline void uart_write_reg(u32 offset, u32 value)
{
    iowrite32(value, uart_base + offset);
}

static inline u32 uart_read_reg(u32 offset)
{
    return ioread32(uart_base + offset);
}

// Initialize UART hardware
static int uart_hw_init(void)
{
    u32 temp;
    
    // Request GPIO pins
    if (gpio_request(GPIO_UART_TXD, "uart_txd") < 0) {
        pr_err("Failed to request GPIO %d\n", GPIO_UART_TXD);
        return -ENODEV;
    }
    
    if (gpio_request(GPIO_UART_RXD, "uart_rxd") < 0) {
        pr_err("Failed to request GPIO %d\n", GPIO_UART_RXD);
        gpio_free(GPIO_UART_TXD);
        return -ENODEV;
    }
    
    // Set GPIO function to UART (ALT0)
    gpio_direction_output(GPIO_UART_TXD, 1);
    gpio_direction_input(GPIO_UART_RXD);
    
    // Request memory region
    if (!request_mem_region(UART0_BASE, UART_SIZE, DEVICE_NAME)) {
        pr_err("Failed to request memory region\n");
        gpio_free(GPIO_UART_TXD);
        gpio_free(GPIO_UART_RXD);
        return -EBUSY;
    }
    
    // Map memory
    uart_base = ioremap(UART0_BASE, UART_SIZE);
    if (!uart_base) {
        pr_err("Failed to map UART memory\n");
        release_mem_region(UART0_BASE, UART_SIZE);
        gpio_free(GPIO_UART_TXD);
        gpio_free(GPIO_UART_RXD);
        return -ENOMEM;
    }
    
    // Disable UART
    uart_write_reg(UART_CR, 0);
    
    // Clear interrupts
    uart_write_reg(UART_ICR, 0x7FF);
    
    // Set baud rate to 115200 (assuming 48MHz clock)
    // IBRD = 48000000 / (16 * 115200) = 26.041... -> 26
    // FBRD = (0.041... * 64) + 0.5 = 3
    uart_write_reg(UART_IBRD, 26);
    uart_write_reg(UART_FBRD, 3);
    
    // Set line control: 8N1, enable FIFOs
    uart_write_reg(UART_LCRH, UART_LCRH_WLEN_8 | UART_LCRH_FEN);
    
    // Enable interrupts for RX
    uart_write_reg(UART_IMSC, UART_INT_RX);
    
    // Enable UART, RX, and TX
    uart_write_reg(UART_CR, UART_CR_UARTEN | UART_CR_RXE | UART_CR_TXE);
    
    // Request IRQ
    if (request_irq(uart_irq, uart_interrupt, IRQF_SHARED, DEVICE_NAME, &uart_cdev)) {
        pr_err("Failed to request IRQ %d\n", uart_irq);
        uart_hw_cleanup();
        return -ENODEV;
    }
    
    pr_info("UART hardware initialized successfully\n");
    return 0;
}

// Cleanup UART hardware
static void uart_hw_cleanup(void)
{
    if (uart_base) {
        // Disable UART
        uart_write_reg(UART_CR, 0);
        
        // Clear interrupts
        uart_write_reg(UART_ICR, 0x7FF);
        uart_write_reg(UART_IMSC, 0);
        
        iounmap(uart_base);
        uart_base = NULL;
    }
    
    if (uart_irq >= 0) {
        free_irq(uart_irq, &uart_cdev);
    }
    
    release_mem_region(UART0_BASE, UART_SIZE);
    gpio_free(GPIO_UART_TXD);
    gpio_free(GPIO_UART_RXD);
}

// Interrupt handler
static irqreturn_t uart_interrupt(int irq, void *dev_id)
{
    u32 status;
    unsigned long flags;
    char data;
    int handled = 0;
    
    spin_lock_irqsave(&uart_lock, flags);
    
    status = uart_read_reg(UART_MIS);
    
    // Handle RX interrupt
    if (status & UART_INT_RX) {
        while (!(uart_read_reg(UART_FR) & UART_FR_RXFE)) {
            data = uart_read_reg(UART_DR) & 0xFF;
            
            // Add to buffer if not full
            int next_head = (rx_head + 1) % BUFFER_SIZE;
            if (next_head != rx_tail) {
                rx_buffer[rx_head] = data;
                rx_head = next_head;
            }
        }
        
        // Clear RX interrupt
        uart_write_reg(UART_ICR, UART_INT_RX);
        wake_up_interruptible(&rx_wait);
        handled = 1;
    }
    
    // Handle TX interrupt
    if (status & UART_INT_TX) {
        while (!(uart_read_reg(UART_FR) & UART_FR_TXFF) && tx_tail != tx_head) {
            uart_write_reg(UART_DR, tx_buffer[tx_tail]);
            tx_tail = (tx_tail + 1) % BUFFER_SIZE;
        }
        
        if (tx_tail == tx_head) {
            // Disable TX interrupt when buffer empty
            u32 imsc = uart_read_reg(UART_IMSC);
            uart_write_reg(UART_IMSC, imsc & ~UART_INT_TX);
        }
        
        uart_write_reg(UART_ICR, UART_INT_TX);
        wake_up_interruptible(&tx_wait);
        handled = 1;
    }
    
    spin_unlock_irqrestore(&uart_lock, flags);
    
    return handled ? IRQ_HANDLED : IRQ_NONE;
}

// File operations
static int uart_open(struct inode *inode, struct file *file)
{
    pr_info("UART device opened\n");
    return 0;
}

static int uart_release(struct inode *inode, struct file *file)
{
    pr_info("UART device closed\n");
    return 0;
}

static ssize_t uart_read(struct file *file, char __user *buffer, size_t len, loff_t *pos)
{
    unsigned long flags;
    int bytes_read = 0;
    char temp_buffer[BUFFER_SIZE];
    int available;
    
    if (len == 0)
        return 0;
    
    // Wait for data
    if (wait_event_interruptible(rx_wait, rx_head != rx_tail))
        return -ERESTARTSYS;
    
    spin_lock_irqsave(&uart_lock, flags);
    
    // Copy data from circular buffer
    while (rx_tail != rx_head && bytes_read < len && bytes_read < BUFFER_SIZE) {
        temp_buffer[bytes_read] = rx_buffer[rx_tail];
        rx_tail = (rx_tail + 1) % BUFFER_SIZE;
        bytes_read++;
    }
    
    spin_unlock_irqrestore(&uart_lock, flags);
    
    // Copy to user space
    if (bytes_read > 0) {
        if (copy_to_user(buffer, temp_buffer, bytes_read)) {
            return -EFAULT;
        }
    }
    
    return bytes_read;
}

static ssize_t uart_write(struct file *file, const char __user *buffer, size_t len, loff_t *pos)
{
    unsigned long flags;
    int bytes_written = 0;
    char temp_buffer[BUFFER_SIZE];
    int space_available;
    u32 imsc;
    
    if (len == 0)
        return 0;
    
    if (len > BUFFER_SIZE)
        len = BUFFER_SIZE;
    
    // Copy from user space
    if (copy_from_user(temp_buffer, buffer, len))
        return -EFAULT;
    
    spin_lock_irqsave(&uart_lock, flags);
    
    // Add data to TX buffer
    while (bytes_written < len) {
        int next_head = (tx_head + 1) % BUFFER_SIZE;
        if (next_head == tx_tail) {
            // Buffer full, wait
            break;
        }
        
        tx_buffer[tx_head] = temp_buffer[bytes_written];
        tx_head = next_head;
        bytes_written++;
    }
    
    // Enable TX interrupt to start transmission
    if (bytes_written > 0) {
        imsc = uart_read_reg(UART_IMSC);
        uart_write_reg(UART_IMSC, imsc | UART_INT_TX);
    }
    
    spin_unlock_irqrestore(&uart_lock, flags);
    
    return bytes_written;
}

// Module initialization
static int __init uart_module_init(void)
{
    int result;
    
    pr_info("Initializing RPi4 UART module\n");
    
    // Allocate device number
    result = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (result < 0) {
        pr_err("Failed to allocate device number: %d\n", result);
        return result;
    }
    major_number = MAJOR(dev_number);
    
    // Initialize character device
    cdev_init(&uart_cdev, &uart_fops);
    uart_cdev.owner = THIS_MODULE;
    
    // Add character device
    result = cdev_add(&uart_cdev, dev_number, 1);
    if (result < 0) {
        pr_err("Failed to add character device: %d\n", result);
        unregister_chrdev_region(dev_number, 1);
        return result;
    }
    
    // Create device class
    uart_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(uart_class)) {
        pr_err("Failed to create device class\n");
        cdev_del(&uart_cdev);
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(uart_class);
    }
    
    // Create device
    uart_device = device_create(uart_class, NULL, dev_number, NULL, DEVICE_NAME);
    if (IS_ERR(uart_device)) {
        pr_err("Failed to create device\n");
        class_destroy(uart_class);
        cdev_del(&uart_cdev);
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(uart_device);
    }
    
    // Initialize hardware
    result = uart_hw_init();
    if (result < 0) {
        device_destroy(uart_class, dev_number);
        class_destroy(uart_class);
        cdev_del(&uart_cdev);
        unregister_chrdev_region(dev_number, 1);
        return result;
    }
    
    pr_info("RPi4 UART module loaded successfully. Major number: %d\n", major_number);
    return 0;
}

// Module cleanup
static void __exit uart_module_exit(void)
{
    pr_info("Cleaning up RPi4 UART module\n");
    
    uart_hw_cleanup();
    
    device_destroy(uart_class, dev_number);
    class_destroy(uart_class);
    cdev_del(&uart_cdev);
    unregister_chrdev_region(dev_number, 1);
    
    pr_info("RPi4 UART module unloaded\n");
}

module_init(uart_module_init);
module_exit(uart_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("UART Driver for Raspberry Pi 4");
MODULE_VERSION("1.0");