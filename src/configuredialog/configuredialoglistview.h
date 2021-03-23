/*
 *   kmail: KDE mail client
 */
#pragma once

#include <QTreeWidget>

class ListView : public QTreeWidget
{
    Q_OBJECT
public:
    explicit ListView(QWidget *parent = nullptr);

Q_SIGNALS:
    void addHeader();
    void removeHeader();

protected:
    void resizeEvent(QResizeEvent *e) override;
    void showEvent(QShowEvent *e) override;

private:
    void slotContextMenu(const QPoint &pos);
    void resizeColums();
};

