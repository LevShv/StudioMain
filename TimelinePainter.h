#pragma once
#include <QQuickPaintedItem>
#include <QPainter>

class TimelinePainter : public QQuickPaintedItem {
    Q_OBJECT
        Q_PROPERTY(int countOfBeats READ countOfBeats WRITE setCountOfBeats NOTIFY countOfBeatsChanged)
        Q_PROPERTY(qreal beatWidth READ beatWidth WRITE setBeatWidth NOTIFY beatWidthChanged)
        Q_PROPERTY(qreal contentX READ contentX WRITE setContentX NOTIFY contentXChanged)
        Q_PROPERTY(qreal bpm READ bpm WRITE setBpm NOTIFY bpmChanged)
        Q_PROPERTY(qreal rulerHeight READ rulerHeight WRITE setRulerHeight NOTIFY rulerHeightChanged)

public:
    TimelinePainter(QQuickItem* parent = nullptr);

    int countOfBeats() const { return m_countOfBeats; }
    void setCountOfBeats(int count);

    qreal beatWidth() const { return m_beatWidth; }
    void setBeatWidth(qreal width);

    qreal contentX() const { return m_contentX; }
    void setContentX(qreal x);

    qreal bpm() const { return m_bpm; }
    void setBpm(qreal bpm);

    qreal rulerHeight() const { return m_rulerHeight; }
    void setRulerHeight(qreal height);

    void paint(QPainter* painter) override;

signals:
    void countOfBeatsChanged();
    void beatWidthChanged();
    void contentXChanged();
    void bpmChanged();
    void rulerHeightChanged();

private:
    int m_countOfBeats = 100;
    qreal m_beatWidth = 40;
    qreal m_contentX = 0;
    qreal m_bpm = 120;
    qreal m_rulerHeight = 50;
};
