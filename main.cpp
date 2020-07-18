#include "delaydefine.h"
#include "layerthread.h"
#include "flowcontrol.h"

int main()
{
#if 1
    initFlowControl();

    runFlowControl();

    finishFlowControl();

#else

    // ���߳�ִ�о������ʹ�ô˷���
    initLayerResource();

    while (true)
    {
        Sleep(1); // ����ֱ�ӿ���

        // �жϾ�����߳�
        if (judgeConvolutionThreads())
        {
            // һ��������
            if (current_layer >= MAX_LAYER) // �����������
                break;
        }
    }
#endif

    return 0;
}
