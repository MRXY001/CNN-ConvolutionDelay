#include <algorithm>
#include "delaydefine.h"
#include "layerthread.h"

int main()
{
    initLayerResource();

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

    return 0;
}
