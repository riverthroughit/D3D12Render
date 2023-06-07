#include "D3D12App.h"
#include<DirectXMath.h>

D3D12App::D3D12App(QWidget *parent)
    : QWidget(parent), timerID(startTimer(2, Qt::PreciseTimer)),D3D12Render((HWND)winId())
{
    ui.setupUi(this);
    //允许DX渲染
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
	static int frameCnt = 0;	//总帧数
	static float timeElapsed = 0.0f;	//流逝的时间 0到1
	static float fps = 0;//当前fps
	++frameCnt;	//每帧++，经过一秒后其即为FPS值
	//判断模块
	if (gameTimer.totalTime() - timeElapsed >= 1.0f){//一旦>=0，说明刚好过一秒
		fps = (float)frameCnt;//每秒多少帧
		//为计算下一组帧数值而重置
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
    setCursor(Qt::ClosedHandCursor);//变为拳头状
    
    //if (++mouseMoveTimes != 20)//降低该事件调用频率 否则change太多
    //    return;
    //else
    //    mouseMoveTimes = 0;

    int nowX = event->x(), nowY = mClientHeight - event->y();
    if (nowX >= mClientWidth || nowX < 0 ||
        nowY >= mClientHeight || nowY < 0) {//超出图片区域
        lastX = -1;
        return;
    }

    if (lastX == -1) {//第一次调用(或刚从外界移动到图片内)
        lastX = nowX, lastY = nowY;
        return;
    }

    //认为鼠标移动改变的为角度从 0.25的映射关系 将角度转化为弧度
    float dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(nowX - lastX));
    float dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(nowY - lastY));

    //更改相机位置
    mCamera.setTheta(mCamera.getTheta() + dy);
    mCamera.setPhi(mCamera.getPhi() - dx);

    lastX = nowX, lastY = nowY;
}

void D3D12App::mouseReleaseEvent(QMouseEvent* event){
    lastX = -1, lastY = -1;//可能为拖拽图片结束
    setCursor(Qt::ArrowCursor);//变回箭头状
}

void D3D12App::wheelEvent(QWheelEvent* event){
    int wheelDis = event->delta() / 24;//滚轮滑动次数
    //0.005的映射关系
    mCamera.setRadius(mCamera.getRadius() - 0.05f * static_cast<float>(wheelDis));
}

void D3D12App::keyPressEvent(QKeyEvent* event){

}

