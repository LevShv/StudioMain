#include "timelinepainter.h"
#include <QPainter>
#include <QPen>
#include <QString>
#include <cmath>
#include <QDebug>

TimelinePainter::TimelinePainter(QQuickItem* parent) : QQuickPaintedItem(parent) {
    setRenderTarget(QQuickPaintedItem::FramebufferObject);
}

void TimelinePainter::setCountOfBeats(int count) {
    if (m_countOfBeats != count) {
        m_countOfBeats = count;
        emit countOfBeatsChanged();
        update();
    }
}

void TimelinePainter::setBeatWidth(qreal width) {
    if (m_beatWidth != width) {
        m_beatWidth = width;
        emit beatWidthChanged();
        update();
    }
}

void TimelinePainter::setContentX(qreal x) {
    if (m_contentX != x) {
        m_contentX = x;
        emit contentXChanged();
        update();
    }
}

void TimelinePainter::setBpm(qreal bpm) {
    if (m_bpm != bpm) {
        m_bpm = bpm;
        emit bpmChanged();
        update();
    }
}

void TimelinePainter::setRulerHeight(qreal height) {
    if (m_rulerHeight != height) {
        m_rulerHeight = height;
        emit rulerHeightChanged();
        update();
    }
}

void TimelinePainter::paint(QPainter* painter) {
    painter->setRenderHint(QPainter::Antialiasing, false);
    QPen pen(QColor("#444"));
    pen.setWidthF(1.0);
    painter->setPen(pen);
    painter->setFont(QFont("sans-serif", m_beatWidth > 15 ? 10 : 8));

    int groupSize = m_beatWidth > 40 ? 1 : m_beatWidth > 20 ? 2 : m_beatWidth > 10 ? 4 : 8;
    int startBeat = std::floor(m_contentX / m_beatWidth / groupSize) * groupSize;
    // Приводим второй аргумент std::min к int
    int endBeat = std::min(m_countOfBeats, static_cast<int>(std::ceil((m_contentX + width()) / m_beatWidth / groupSize)) * groupSize);

    for (int beatIndex = startBeat; beatIndex <= endBeat; beatIndex += groupSize) {
        qreal x = beatIndex * m_beatWidth - m_contentX;

        // Линейка
        painter->drawLine(QPointF(x, 0), QPointF(x, m_rulerHeight));

        // Вертикальные полосы
        painter->drawLine(QPointF(x, m_rulerHeight), QPointF(x, height()));

        // Метки
        if (m_beatWidth > 10) {
            QString text = QString::number(beatIndex + 1);
            painter->drawText(QPointF(x + 4, m_rulerHeight / 2), text); // Линейка
            painter->drawText(QPointF(x + 4, m_rulerHeight + 12), text); // Сетка
        }
    }

    // Отладка
    qDebug() << "TimelinePainter painting, startBeat:" << startBeat << "endBeat:" << endBeat << "contentX:" << m_contentX;
}