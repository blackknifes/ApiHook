# ApiHook
ApiHook demo
---
MutipleThread Supported			支持多线程（线程安全）<br>
No Use Thread Locker			没有使用多线程加锁，直接将原始api头部地址拷贝到新地址，然后在新地址末尾执行一个jmp命令跳转的原始地址+5的位置，因此即使不加锁也不影响多线程操作与调用。
