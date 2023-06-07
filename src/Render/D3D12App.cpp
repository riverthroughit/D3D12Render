#include "D3D12App.h"
#include<DirectXMath.h>

D3D12App::D3D12App(QWidget *parent)
    : QWidget(parent), timerID(startTimer(2, Qt::PreciseTimer)),D3D12Render((HWND)winId())
{
    ui.setupUi(this);
    //����DX��Ⱦ
    QWidget::setAttribute(Qt::WA_PaintOnScreen); 
    initDirect3D();

}

D3D12App::~D3D12App()
{}

void D3D12App::timerEvent(QTimerEvent* event) {
	update();
}

void D3D12App::paintEvent(QPaintEvent* event) {
	gameTimer.tick();
	printFPS(calculateFrameState());
	updateData();
	draw();
}

void D3D12App::resizeEvent(QResizeEvent* event){
    mClientWidth = width();
    mClientHeight = height();
    onResize();
}

float D3D12App::calculateFrameState()
{
	static int frameCnt = 0;	//��֡��
	static float timeElapsed = 0.0f;	//���ŵ�ʱ�� 0��1
	static float fps = 0;//��ǰfps
	++frameCnt;	//ÿ֡++������һ����伴ΪFPSֵ
	//�ж�ģ��
	if (gameTimer.totalTime() - timeElapsed >= 1.0f){//һ��>=0��˵���պù�һ��
		fps = (float)frameCnt;//ÿ�����֡
		//Ϊ������һ��֡��ֵ������
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
	return fps;
}

void D3D12App::printFPS(float fps) {
	QString s = QString("%1 %2%3").arg("renderer").arg("FPS:").arg(fps);
	setWindowTitle(s);
}

void D3D12App::mousePressEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton)
		mouseLeftPressed = true;
	else
		mouseLeftPressed = false;
}

void D3D12App::mouseMoveEvent(QMouseEvent* event) {
    setCursor(Qt::ClosedHandCursor);//��Ϊȭͷ״
    
    //if (++mouseMoveTimes != 20)//���͸��¼�����Ƶ�� ����change̫��
    //    return;
    //else
    //    mouseMoveTimes = 0;

    int nowX = event->x(), nowY = mClientHeight - event->y();
    if (nowX >= mClientWidth || nowX < 0 ||
        nowY >= mClientHeight || nowY < 0) {//����ͼƬ����
        lastX = -1;
        return;
    }

    if (lastX == -1) {//��һ�ε���(��մ�����ƶ���ͼƬ��)
        lastX = nowX, lastY = nowY;
        return;
    }

    //��Ϊ����ƶ��ı��Ϊ�Ƕȴ� 0.25��ӳ���ϵ ���Ƕ�ת��Ϊ����
    float dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(nowX - lastX));
    float dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(nowY - lastY));

    //�������λ��
    mCamera.setTheta(mCamera.getTheta() + dy);
    mCamera.setPhi(mCamera.getPhi() - dx);

    lastX = nowX, lastY = nowY;
}

void D3D12App::mouseReleaseEvent(QMouseEvent* event){
    lastX = -1, lastY = -1;//����Ϊ��קͼƬ����
    setCursor(Qt::ArrowCursor);//��ؼ�ͷ״
}

void D3D12App::wheelEvent(QWheelEvent* event){
    int wheelDis = event->delta() / 24;//���ֻ�������
    //0.005��ӳ���ϵ
    mCamera.setRadius(mCamera.getRadius() - 0.05f * static_cast<float>(wheelDis));
}

void D3D12App::keyPressEvent(QKeyEvent* event){

}

