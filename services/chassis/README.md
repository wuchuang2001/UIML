## 使用步骤

1. 打开CubeMX工程,添加DSP库
   - 打开Select Components
  
   ![选择包](README-IMG/选择包.png)
   - 选择DSP Library
  
	![引用DSP库](README-IMG/引用DSP库.png)
   - 返回cubeMX,添加DSP Library

   ![选择DSP库](README-IMG/选择DSP库.png)

   - 将所有库拷贝到工程文件夹下

   ![拷贝所有库到工程](README-IMG/拷贝所有库到工程.png)
	
2. 打开Keil工程
   - 在全局宏定义处添加 `,ARM_MATH_CM4,__TARGET_FPU_VFP,__FPU_PRESENT=1U` ,以打开FPU
   
   ![添加全局宏定义](README-IMG/添加全局宏定义.png)
   - 修改Keil工程包含目录，删除原有的`arm_cortexM4l_math.lib`,添加\Drivers\CMSIS\Lib\ARM目录下`arm_cortexM4lf_math.lib`

   ![添加math库](README-IMG/添加math库.png)