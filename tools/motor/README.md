# �����

## ���

����Ŀʵ����һ������ĳ����࣬��C++���������ƣ������౻�������ĺ���ָ���ָ������Ķ���ĺ�����ֻ��Ҫ���ø���ĺ��������������ʵ����������ξ���ʵ�ֵ�

## �����ļ�

��Ŀ����һ��`motor.c/h`�ļ��Լ���Ӧ��`m3508.c`��`m2006.c`��`m6020.c`��`dc_motor.c`��`servo.c`�����ļ���ʹ��ǰ��`motor`�ļ�������Ҫ�������ļ���������Ŀ�м���

## ������

��Ŀ�Ա�׼���ļ�`stdint.h`��`stdlib.h`��`string.h`��������ʹ�������е�`abs`�����Լ�`uint32_t`�Ȼ������ͣ�ͬʱҲ��freertos��`cmsis_os.h`�ͱ���Ŀ��`config.h`���������ֱ�ʹ���˶�ȡ���ñ���Ĳ�����ʼ������Լ�freertos��������ʱ��

## �����ṹ����

### ����

- ����ֻ�ṩ�������麯���ӿڣ����ṩ�κ����ԣ��������ñ���ѡ����������ͽ��г�ʼ��
- ������麯��ʱ��������ķ�������С���������ϣ�����ĳЩû��û��ָ��ĺ���ָ���Ĭ��ָ��պ���(��ʲôҲ�������)

### ����

- ����̳и���(�ṹ���ǿ������ת��)������`.c`�ļ��л���������е�һЩ˽������
- �������ʼ�����ʹ����ĺ���ָ��ָ�������ʵ��
- Ŀǰ���е������У�`m3508`��`m2006`��`m6020`��`ֱ�����`��`���`

## ��`sys_conf.h`�е�����

```c
{"motor", CF_DICT{
	{"type", "M3508"}, //������������
	...                //����������Ҫ��������Ϣ����
	CF_DICT_END
}},
```
- ����������������Ϣ�����[�󽮵��](motor_can/README.md)��[pwm���](motor_pwm/README.md)

## �ӿ�˵��

1. `Motor* Motor_Init(ConfItem* dict)`
   
   ����ݴ���������е������Զ�ƥ���Ӧ�������ʼ������ͬʱֻ���ظ������ӿڲ���¶ʵ��ϸ�ڡ�ʹ��ʾ����

	```c
	Motor* motor = NULL;
	motor = Motor_Init(dict);
	```

2. ������е�`changeMode`�ӿ�

	ͨ���˺������Ը��ĵ���Ŀ���ģʽ����ֱ�ӵ���������������ٶ�pid���ơ������Ƕ�pid���ơ�ʹ��ʾ����

	```c
	Motor* motor = NULL;
	motor = Motor_Init(dict);
	motor->changeMode(motor, MOTOR_TORQUE_MODE);//����ģʽ
	motor->changeMode(motor, MOTOR_SPEED_MODE);//�ٶ�ģʽ
	motor->changeMode(motor, MOTOR_ANGLE_MODE);//�Ƕ�ģʽ
	```

3. ������е�`setTarget`�ӿ�

	ͨ���˺������Ը��ĵ����Ŀ��ֵ��������Ϊ�Ƕ���������£������ģʽ�Ĳ�ͬĿ��ֵ����������Ҳ������ͬ�������ٶ�ģʽ�����õ�Ŀ��ֵ����Ŀ���ٶȣ��ڽǶ�ģʽ�����õ�Ŀ��ֵ����Ŀ��Ƕȡ�����Ƕ���࣬��Ϊ��ֻ�нǶ�ģʽ���������õľ���Ŀ��Ƕȡ�ʹ��ʾ����

	```c
	Motor* motor = NULL;
	motor = Motor_Init(dict);
	motor->setTarget(motor, 60);
	```

4. ������е�`setStartAngle`�ӿ�

	ͨ���˺����������õ�������(����ת��)�Ŀ�ʼ�Ƕȡ��ڸ��ϵ�ʱ����ĽǶȲ�һ��������㴦������ͨ���˺������õ�������ĳ�ʼ�Ƕȣ�֮�����pid�������´ﵽ���е�Ч����ʹ��ʾ����

	```c
	Motor* motor = NULL;
	motor = Motor_Init(dict);
	motor->setStartAngle(motor, -60);//��λ����
	```
5. ������е�`getData`�ӿ�

	ͨ���˺������Ի�ȡ����Ĳ������ݡ�Ŀǰ���ŵ������У�ת�Ӿ�����������ĽǶȺ������������õ������ܽǶȡ�ʹ��ʾ����

	```c
	Motor* motor = NULL;
	motor = Motor_Init(dict);
	motor->getData(motor, "angle");//��ȡת�ӽǶȣ���λ����
	motor->getData(motor, "totalAngle");//��ȡ������ܽǶȣ���λ����
	```

6. ������е�`stop`�ӿ�

	ͨ���˺�������ʹ������뼱ͣģʽ���ҽ���������ͨ��������Ƭ���˳���ʹ��ʾ����

	```c
	Motor* motor = NULL;
	motor = Motor_Init(dict);
	motor->stop(motor);
	```

## ע������

1. �ڵ��Թ�����ֱ�Ӳ鿴��������ǲ��Ǳ�¶�����˽�����Եģ������ڵ��Թ����в鿴�����˽�����ԣ�ֻ��Ҫ�ڵ��Ա������ڶԸ������ǿ������ת�����ɡ��磺

	![��������鿴](README-IMG/��������鿴.png)