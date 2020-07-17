#include <stdio.h>
#include <pthread.h>
#include <algorithm>
#include <queue>
#include <windows.h>
#include "convolution.h"
#include "delaydefine.h"


int current_layer = 0;   // ��ǰ�����ڵڼ���
int finished_kernel = 0; // ��ǰ�������kernel����
std::vector<pthread_t*> conv_thread;   // ���̶߳���
std::vector<FeatureMap*> feature_maps; // ÿ��ͼ


/**
 * ��ȡ����˵�����
 * ��ʵÿһ������������һ�������
 * 3*3*3 -> 3*3*8 -> 16 -> 32 -> 32 -> 32...
 */
inline int getKernelCount(int layer)
{
    switch (layer)
    {
    case 0:
        return 3;
    case 1:
        return 8;
    case 2:
        return 16;
    default:
        return 32;
    }
}


/**
 * ��������˽��о�����߳�
 * @return ���������chnnel��ͼ���ϲ���1��
 */
void *convolutionThread(void *_arg)
{
    pthread_detach(pthread_self()); // unjoinable�������������н������˳�
    ConvThreadArg* arg = (ConvThreadArg*) _arg;
    FeatureMap* map = arg->map;
    Kernel* kernel = arg->kernel;
    printf("> ��ʼ������߳�: kernel: %d\n", arg->k_indx);
    if (map)
        printf("    ����ͼ: %d * %d * %d\n", map->side, map->side, map->channel);
    if (kernel)
        printf("    �����: %d * %d * %d\n", kernel->side, kernel->side, kernel->channel);

    // ��ʼ���
    FeatureMap *result = convolution(map, kernel);

    // ���Ҫ���ݵ���һ��
    feature_maps.push_back(result);
    finished_kernel++;

    printf("- �������: kernel: %d\n", arg->k_indx);
    // �ͷ���Դ
    delete arg;
    delete map;
    pthread_exit(0);
    return 0;
}

/**
 * �ͷ���һ��ʱnew����������
 */
void releasePrevLayer()
{
    // �ͷ���һ��ָ����
    while (!feature_maps.empty())
    {
        delete feature_maps.back();
        feature_maps.pop_back();
    }

    // �ͷ���һ����߳�
    while (!conv_thread.empty())
    {
        pthread_join(*conv_thread.back(), NULL);
        conv_thread.pop_back();
    }
}

/**
 * �ж϶��߳̾�����������
 * @return falseʱ�߳�δ������trueʱ��ʾ�߳̽���
 */
bool judgeConvolutionThreads()
{
    int kernel_count = getKernelCount(current_layer); // ��һ���kernel����
    // ����ȷ�� finished_kernel == kernel_count == feature_maps.count(), �� > 0
    if (finished_kernel < kernel_count)
        return false;

    // �ϲ�FeatureMap
    FeatureMap* map = NULL;
    int channel_count = kernel_count; // ��һ���channel���� = ��һ���kernel���� = ��һ������map������
    if (current_layer <= 0) // ����ʹ�ã�������ô��Ĳ���
    {
        map = feature_maps.front();
        feature_maps.clear(); // map����Ҫ�õ�������delete
        printf("��ʼ����ͼ��%d * %d * %d\n", map->side, map->side, map->channel);
    }
    else
    {
        // �ϲ���һ��ÿ��kernel��FeatureMap
        std::vector<FeatureMap*> prev_map = feature_maps;
        // �����߳�kernel˳�������������ϲ�
        std::sort(prev_map.begin(), prev_map.end(), [=](FeatureMap* a, FeatureMap* b){
            return a->kernel < b->kernel;
        });

        int side = prev_map.front()->side;
        map = new FeatureMap(0, side, channel_count);
        printf("�ϲ�����ͼ��%d * %d * %d\n", side, side, channel_count);
        for (int i = 0; i < channel_count; i++)
        {
            // memcpy(map->map[i], prev_map.at(i)->map[0], sizeof(INT8)*side*side); // ���������ڴ棬�޷�cpy
            INT8*** p_map = prev_map.at(i)->map;
            for (int y = 0; y < side; y++)
            {
                for (int x = 0; x < side; x++)
                {
                    map->map[y][x][i] = p_map[y][x][0];
                }
            }
        }

        releasePrevLayer();
    }

    // ��� MAX_LAYER ��(Ŀǰ32)
    if (current_layer >= MAX_LAYER)
    {
        // �����������ĵ�����ͼmap�С���ʱû�����
        printf("ȫ�����н���");
        return true;
    }

    // ������һ��
    current_layer++;
    printf("\n================ �����%d�� ================\n\n", current_layer);
    kernel_count = getKernelCount(current_layer); // ��ǰ���kernel����
    printf("kernel count = %d\n", kernel_count);

    // �������߳�
    finished_kernel = 0; // ����ɵ��߳���������Ϊ0
    for (int k = 0; k < kernel_count; k++)
    {
        // ����ȫ�����ݣ�ͼ+�ˣ������߳̽�����ʱ��delete��
        ConvThreadArg* arg = new ConvThreadArg(current_layer, k+1, new FeatureMap(k+1, map), new Kernel(KERNEL_SIDE, channel_count));

        // ������̡߳��ò����߳�ȫ��������ͳһ�ͷ�
        pthread_t* thread = new pthread_t;
        conv_thread.push_back(thread);
        int ret = pthread_create(thread, NULL, convolutionThread, (void*)arg);
        if (ret != 0)
            printf("pthread_create error: %d\n", ret);
    }
    return false;
}

int main()
{
    // ���δ���ͨ��
    current_layer = 0;   // ��0��
    finished_kernel = 3; // ��0���kernel��=��1���channel=3
    feature_maps.push_back(new FeatureMap(0, MAP_SIDE_MAX, MAP_CHANNEL_DEFULT)); // Ĭ��224*224*3��ͼ

    // ��ѭ��һֱ�ȵ�picker
    while (true)
    {
        Sleep(1); // ����ֱ�ӿ���

        // �жϾ�����߳�
        if (judgeConvolutionThreads())
        {
            // �����������
            if (current_layer >= MAX_LAYER)
                break;

            // ֻ��ĳһ�����

        }
    }

    pthread_exit(NULL);
    return 0;
}
