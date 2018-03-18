#ifndef __DEBUG_H__
#define __DEBUG_H__

typedef enum{
	EV_JOIN_REQ,
	EV_DATA_UNCONFIRMED_UP,
	EV_DATA_CONFIRMED_UP,
	EV_RXCOMPLETE,
	EV_TXCOMPLETE,
	EV_TX_START,
	EV_DATA_PREPARE_TO_SEND,
	EV_LAST
}en_Event;

extern unsigned char* enEventName[];

#define EV_JOIN_REQ		(1 < 0)
#define EV_JOIN_REQ		(1 < 0)
#define EV_JOIN_REQ		(1 < 0)

//#define DEBUG_DATA	(1 < 0)
//#define DEBUG_EVENT	(1 < 1)
//#define DEBUG_BUG	(1 < 2)
//#define DEBUG_WARNNING	(1 < 3)
//#define DEBUG_INFO	(1 < 4)
//#define DEBUG_DATA	(1 < 5)
//#define DEBUG_DATA	(1 < 6)
//#define DEBUG_DATA	(1 < 7)
void debug_output_data(uint8_t *data,int len);
//#define DEBUG_MASK	DEBUG_DATA|DEBUG_EVENT|DEBUG_BUG|DEBUG_WARNNING
#if defined(DEBUG_DATA)
#define DEBUG_OUTPUT_DATA(data,len)		debug_output_data(data,len);
#else
#define DEBUG_OUTPUT_DATA(data,len)
#endif
#if defined(DEBUG_EVENT)
#define DEBUG_OUTPUT_EVENT(chip,ev)	printk("%d: %s\r\n",chip,enEventName[ev]);
#else
#define DEBUG_OUTPUT_EVENT(chip,ev)
#endif
#if defined(DEBUG_INFO)
#define DEBUG_OUTPUT_INFO(fmt,arg...)	printk(fmt, ##arg)
#else
#define DEBUG_OUTPUT_INFO(fmt,arg...)
#endif

//#endif
/*KERN_EMERG
Used for emergency messages, usually those that precede a crash.
用于突发性事件的消息，通常在系统崩溃之前报告此类消息。
KERN_ALERT
A situation requiring immediate action.
在需要立即操作的情况下使用此消息。
KERN_CRIT
Critical conditions, often related to serious hardware or software failures.
用于临界条件下，通常遇到严重的硬软件错误时使用此消息。
KERN_ERR
Used to report error conditions; device drivers often use KERN_ERR to report hardware difficulties.
用于报告错误条件；设备驱动经常使用KERN_ERR报告硬件难题。
KERN_WARNING
Warnings about problematic situations that do not, in themselves, create serious problems with the system.
是关于问题状况的警告，一般这些状况不会引起系统的严重问题。
KERN_NOTICE
Situations that are normal, but still worthy of note. A number of security-related conditions are reported at this level.
该级别较为普通，但仍然值得注意。许多与安全性相关的情况会在这个级别被报告。
KERN_INFO
Informational messages. Many drivers print information about the hardware they find at startup time at this level.
信息消息。许多驱动程序在启动时刻用它来输出获得的硬件信息。
KERN_DEBUG
*/
#endif
