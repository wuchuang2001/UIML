#include "filter.h"

//一阶卡尔曼滤波结构体
typedef struct
{
	Filter filter;

	float xLast; //上一时刻的最优结果  X(k|k-1)
	float xPre;  //当前时刻的预测结果  X(k|k-1)
	float xNow;  //当前时刻的最优结果  X(k|k)
	float pPre;  //当前时刻预测结果的协方差  P(k|k-1)
	float pNow;  //当前时刻最优结果的协方差  P(k|k)
	float pLast; //上一时刻最优结果的协方差  P(k-1|k-1)
	float kg;     //kalman增益
	float Q;
	float R;
}KalmanFilter;


Filter* Kalman_Init(ConfItem* dict);
float Kalman_Cala(Filter* filter, float data);

/**
  * @brief  初始化一个卡尔曼滤波器
  * @param  kalman:  滤波器
  * @param  T_Q:系统噪声协方差
  * @param  T_R:测量噪声协方差
  * @retval None
  * @attention R固定，Q越大，代表越信任侧量值，Q无穷代表只用测量值
  *           反之，Q越小代表越信任模型预测值，Q为零则是只用模型预测
  */
Filter* Kalman_Init(ConfItem* dict)
{
	KalmanFilter* kalman = (KalmanFilter*)FILTER_MALLOC_PORT(sizeof(KalmanFilter));
	memset(kalman,0,sizeof(KalmanFilter));

	kalman->filter.cala = Kalman_Cala;
	kalman->xLast = 0;
	kalman->pLast = 1;
	kalman->Q = Conf_GetValue(dict, "t-q", float, 1);
	kalman->R = Conf_GetValue(dict, "t-r", float, 1);

	return (Filter*)kalman;
}


/**
  * @brief  卡尔曼滤波器
  * @param  kalman:  滤波器
  * @param  data:待滤波数据
  * @retval 滤波后的数据
  * @attention Z(k)是系统输入,即测量值   X(k|k)是卡尔曼滤波后的值,即最终输出
  *            A=1 B=0 H=1 I=1  W(K)  V(k)是高斯白噪声,叠加在测量值上了,可以不用管
  *            以下是卡尔曼的5个核心公式
  *            一阶H'即为它本身,否则为转置矩阵
  */
float Kalman_Cala(Filter* filter, float data)
{
    KalmanFilter* kalman = (KalmanFilter*)filter;

	kalman->xPre = kalman->xLast;                            //x(k|k-1) = A*X(k-1|k-1)+B*U(k)+W(K)
	kalman->pPre = kalman->pLast + kalman->Q;                     //p(k|k-1) = A*p(k-1|k-1)*A'+Q
	kalman->kg = kalman->pPre / (kalman->pPre + kalman->R);            //kg(k) = p(k|k-1)*H'/(H*p(k|k-1)*H'+R)
	kalman->xNow = kalman->xPre + kalman->kg * (data - kalman->xPre);  //x(k|k) = X(k|k-1)+kg(k)*(Z(k)-H*X(k|k-1))
	kalman->pNow = (1 - kalman->kg) * kalman->pPre;               //p(k|k) = (I-kg(k)*H)*P(k|k-1)
	kalman->pLast = kalman->pNow;                            //状态更新
	kalman->xLast = kalman->xNow;                            //状态更新
	return kalman->xNow;                                 //输出预测结果x(k|k)
}
