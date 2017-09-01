#ifndef __DEBUG_H__
#define __DEBUG_H__

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
#endif