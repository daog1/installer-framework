#ifndef CLICKFORWARDER_H
#define CLICKFORWARDER_H
#include "packagemanagergui.h"
#include <QtCore/qobject.h>
#include <QPushButton>
#include <QRadioButton>
#include <QtWidgets/qabstractbutton.h>

class QInstaller::LicenseAgreementPage::ClickForwarder : public QObject
{
    Q_OBJECT

public:
    explicit ClickForwarder(QAbstractButton *button)
        : QObject(button)
        , m_abstractButton(button) {}

protected:
    bool eventFilter(QObject *object, QEvent *event)
    {
        if (event->type() == QEvent::MouseButtonRelease) {
            m_abstractButton->click();
            return true;
        }
        // standard event processing
        return QObject::eventFilter(object, event);
    }
private:
    QAbstractButton *m_abstractButton;
};


#endif // CLICKFORWARDER_H
