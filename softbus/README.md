# 软总线

## 简介

本项目实现了一个软总线，可以用于实现多模块间解耦，使多个模块之间通过订阅、发布话题完成数据交互。

## 包含文件

项目仅包含一对`softbus.c/h`文件，使用前将这两个文件添加至项目中即可

## 依赖项

项目不仅对标准库文件`stdint.h`、`stdlib.h`、`string.h`、`stdarg.h`有依赖，也对`vector.h`有依赖，使用了其中的`malloc`、`free`、`memcpy`、`strlen`以及`Vector`、`uint32_t`等基础类型，用户可以通过修改`softbus.c`中的宏定义进行移植替换

## 发布订阅模式

在介绍模块接口之前，首先我们先简单介绍一下发布订阅模式。其实在平时生活中发布订阅模式也是非常常见的场景，比如你打开你的微信公众号，你订阅的作者发布的文章，会发给每个订阅者。在这个场景里，微信公众号的作者就是一个Pulisher，而你就是一个Subscriber，这个公众号就是一个Topic，你收到的文章就是一个Message。如下图。

![发布订阅模式](发布订阅模式.jpg)

其实对于一个Topic不一定只有一个Publisher就以上面的例子来讲不同的作者都可以通过这一个公众号发布Message给所有订阅这个公众号的Subscriber,甚至我们可以简单的理解为Topic就是一个Message的管道，任意一个Publisher都可以通过这个管道传递消息给所有订阅了这个Topic的Subscriber。

接下来简单介绍一下该模式的优点

1. 可以将众多需要通信的子系统解耦，每个子系统都可以独立管理。而且即使部分子系统下线了，也不会影响系统消息的整体管理。
2. 该模式为应用程序提供了关注点分离。每个应用程序都可以专注于其核心功能，而消息传递基础结构负责将消息路由到每个消费者手里。

基于以上优点，软总线使用发布订阅模式进行不同模块之间的数据传输

## 接口使用示例

假设我们有一个模块有一些数据需要其他模块传入，有一些数据需要发布出去供其他模块使用

```c
typedef struct{
	int16_t input1,output1;
	float input2,output2;
}Module;
```

1. 修改依赖

	1. 如果使用freertos应将`softbus.c`中的宏定义改为
	```c
	#define SOFTBUS_MALLOC_PORT(len) pvPortMalloc(len)
	#define SOFTBUS_FREE_PORT(ptr) vPortFree(ptr)
	```
	2. 如果topic字符串平均长度大于20字符，应将`softbus.c`中的宏定义改为
   ```c
	#define SoftBus_Str2Hash(str) SoftBus_Str2Hash_32(str)
	```

2. 发布话题

   ```c
   Module module1;//假设module1里面已有数据
   ```

   模块的输出接口发布相关topic对外传递数据
   
	```c
	//方法1：使用自定义格式数据发布话题，类似串口收发，发送和接收端自行规定协议去解析数据
	uint8_t buffer[7]={0x55};
	buffer[1] = module1.output1;
	buffer[2] = module1.output1 >> 8;
	memcpy(buffer+3,&module1.output2,sizeof(float));
	SoftBus_Publish("buffer",buffer,7);
	
	//方法2：使用映射表发布话题
	SoftBus_PublishMap("\module1\output",{{"output1",&module1.output1,sizeof(int16_t)},
										  {"output2",&module1.output2,sizeof(float)}});
	```

3. 订阅话题
   
   ```c
   Module module2;
   ```

    1)  在回调函数中解析数据

		```c
		//情况一：使用自定义格式数据发布的话题
		void callback(const char* topic, SoftBusFrame* frame)
		{
			if(!strcmp(topic, "\module1\output"))//确定是哪个话题发布的数据
			{
				uint8_t buffer* = frame->data;
				if(buffer[0] == 0x55)//确认帧头
				{
					module2.input1 = (buffer[2] << 8) | buffer[1];
					memcpy(&module2.output2,buffer+3, sizeof(float), sizeof(float));
				}
			}
		}

		//情况二：使用另一种方法发布的话题
		void callback(const char* topic, SoftBusFrame* frame)
		{
			if(!strcmp(topic, "\module1\output"))//确定是哪个话题发布的数据
			{
				SoftBusItem* item = SoftBus_GetItem(frame,"output1");
				if(!item)
					module2.input1 = *(int16_t*)item->data//获取数据帧里的output1对应的数据
				SoftBusItem* item = SoftBus_GetItem(frame,"output2");
				if(!item)
					module2.input2 = *(float*)item->data//获取数据帧里的output2对应的数据
			}
		}
		```
		
	1)  订阅相关话题

		> 注：仅需要订阅一次，之后对应话题发布后会自动的调用注册的回调函数

		```c
		//订阅一个话题
		SoftBus_Subscribe(callback,"\module1\output");
		//订阅多个话题
		SoftBus_MultiSubscribe(callback,{"topic1","topic2"});
		```

## 注意事项

1. 动态添加的数据全部分配自堆区，应保证堆区大小足够

2. 经测试在17个topic下其中一个topic注册了两个回调函数，该模块仍有近30kHz的速率
