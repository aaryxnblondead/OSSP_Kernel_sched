#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

#ifdef CONFIG_UNWINDER_ORC
#include <asm/orc_header.h>
ORC_HEADER;
#endif

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
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

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xc00e2b80, "seq_printf" },
	{ 0x34db050b, "_raw_spin_lock_irqsave" },
	{ 0xd35cce70, "_raw_spin_unlock_irqrestore" },
	{ 0x87a21cb3, "__ubsan_handle_out_of_bounds" },
	{ 0x122c3a7e, "_printk" },
	{ 0x2dbde678, "proc_create" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x7f02188f, "__msecs_to_jiffies" },
	{ 0x15ba50a6, "jiffies" },
	{ 0xc38c83b8, "mod_timer" },
	{ 0xa54c0f5b, "init_task" },
	{ 0x8d522714, "__rcu_read_lock" },
	{ 0x4c03a563, "random_kmalloc_seed" },
	{ 0x8da0819, "kmalloc_caches" },
	{ 0xd0c3484c, "kmalloc_trace" },
	{ 0x69296e5c, "__get_task_comm" },
	{ 0x2469810f, "__rcu_read_unlock" },
	{ 0x82ee90dc, "timer_delete_sync" },
	{ 0xe3e8a148, "proc_remove" },
	{ 0x37a0cba, "kfree" },
	{ 0xc1d9b323, "seq_read" },
	{ 0x7369f212, "seq_lseek" },
	{ 0x8e66928c, "single_release" },
	{ 0x4239c21d, "param_ops_uint" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x46b0be46, "single_open" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xb43f9365, "ktime_get" },
	{ 0xe2fd41e5, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "9957175F9238BD0C553F945");
