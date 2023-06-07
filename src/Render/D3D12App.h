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
    GameTimer mGameTimer;//��ʱ��
    bool mAppPaused = false;//��ǰ�Ƿ���ͣ
    
    int timerID;//qt��ͼ��ʱ��id

    bool mouseLeftPressed = false;//�������
    int lastX = -1, lastY = -1;//�ϴ��϶�����λ��
    int mouseMoveTimes = 0;//����ƶ�����

public:
    D3D12App(QWidget* parent = nullptr);
    virtual ~D3D12App();

protected:
    void timerEvent(QTimerEvent* event)override;
    void paintEvent(QPaintEvent* event)override;
    void resizeEvent(QResizeEvent* event)override; 
    QPaintEngine* paintEngine()const override { return nullptr; };//����qt����Ļ���

    void mousePressEvent(QMouseEvent* event)override;
    void mouseMoveEvent(QMouseEvent* event)override;
    void mouseReleaseEvent(QMouseEvent* event)override;
    void wheelEvent(QWheelEvent* event)override;
    void keyPressEvent(QKeyEvent* event)override;

private:
    float calculateFrameState();
    void printFPS(float fps);
};
