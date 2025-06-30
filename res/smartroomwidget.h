#ifndef SMARTROOMWIDGET_H
#define SMARTROOMWIDGET_H

#include <QWidget>

class QComboBox;
class QListWidget;
class QTableWidget;
class QLabel;
class QPushButton;

class SmartRoomWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SmartRoomWidget(QWidget *parent = nullptr);

private slots:
    void onSearchClicked();
    void parseJsonAndDisplay(const QString &jsonText);

private:
    QComboBox    *buildingBox;
    QComboBox    *timeBox;
    QListWidget  *sectionList;
    QPushButton  *searchButton;
    QTableWidget *resultTable;
    QLabel       *statusLabel;
};

#endif // SMARTROOMWIDGET_H
