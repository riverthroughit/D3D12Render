#pragma once

#include <QtWidgets/QWidget>
#include<QMouseEvent>

#include "ui_D3D12App.h"
#include"D3D12Render.h"

class D3D12App : public QWidget,public D3D12Render
{
    Q_OBJECT


protected:


private:
    Ui::D3D12AppClass ui;
    GameTimer mGameTimer;//计时器
    bool mAppPaused = false;//当前是否暂停
    
    int timerID;//qt绘图定时器id

    bool mouseLeftPressed = false;//左键按下
    int lastX = -1, lastY = -1;//上次拖动鼠标的位置
    int mouseMoveTimes = 0;//鼠标移动次数

public:
    D3D12App(QWidget* parent = nullptr);
    virtual ~D3D12App();

protected:
    void timerEvent(QTimerEvent* event)override;
    void paintEvent(QPaintEvent* event)override;
    void resizeEvent(QResizeEvent* event)override; 
    QPaintEngine* paintEngine()const override { return nullptr; };//屏蔽qt自身的绘制

    void mousePressEvent(QMouseEvent* event)override;
    void mouseMoveEvent(QMouseEvent* event)override;
    void mouseReleaseEvent(QMouseEvent* event)override;
    void wheelEvent(QWheelEvent* event)override;
    void keyPressEvent(QKeyEvent* event)override;

private:
    float calculateFrameState();
    void printFPS(float fps);
};
