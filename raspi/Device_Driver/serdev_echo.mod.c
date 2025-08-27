#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x122c3a7e, "_printk" },
	{ 0xc80a9c61, "__serdev_device_driver_register" },
	{ 0x4781e1, "serdev_device_close" },
	{ 0xefac48f2, "serdev_device_write_buf" },
	{ 0xd51bf3d7, "driver_unregister" },
	{ 0x79a3f4ee, "serdev_device_open" },
	{ 0x676e7cfc, "serdev_device_set_baudrate" },
	{ 0x85710cfe, "serdev_device_set_flow_control" },
	{ 0xc2f74793, "serdev_device_set_parity" },
	{ 0x39ff040a, "module_layout" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Cbrightlight,echodev");
MODULE_ALIAS("of:N*T*Cbrightlight,echodevC*");

MODULE_INFO(srcversion, "46365E36F2B2662FFDA0038");
