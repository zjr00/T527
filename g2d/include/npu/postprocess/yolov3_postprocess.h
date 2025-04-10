#ifndef _YOLOV3_POSTPROCESS_H_
#define _YOLOV3_POSTPROCESS_H_

#ifdef __cplusplus
       extern "C" {
#endif

typedef struct
{
    int xmin; // �������
    int ymin; // ��������
    int xmax; // �Ҳ�����
    int ymax; // �ײ�����
    int label; // ��ǩ�����Σ�����:0�������Σ�����>0��
    float score; // ����ֵ
    int landmark_x[5]; // �������x���ֱ��Ӧ���ۡ����ۡ����ӡ�����࣬���Ҳ�
    int landmark_y[5]; // �������y���ֱ��Ӧ���ۡ����ۡ����ӡ�����࣬���Ҳ�
} BBoxRect_t;

typedef struct
{
    int valid_cnt;                  // ���������
    BBoxRect_t boxes[100];          // �����Ŀ����Ϣ
} Awnn_Result_t;

int yolov3_postprocess(float **tensor, Awnn_Result_t *res, int width, int height, int img_w, int img_h, float thresh);

#ifdef __cplusplus
}
#endif

#endif