/********************************************************************************
** Form generated from reading UI file 'AuthenticationForm.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_AUTHENTICATIONFORM_H
#define UI_AUTHENTICATIONFORM_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_AuthenticationForm
{
public:
    QGroupBox *groupBox;
    QWidget *layoutWidget;
    QHBoxLayout *horizontalLayout_2;
    QPushButton *pushButton_signup;
    QPushButton *pushButton_signin;
    QLabel *label;
    QLabel *label_2;
    QWidget *layoutWidget1;
    QVBoxLayout *verticalLayout;
    QLineEdit *lineEdit_nickname;
    QLineEdit *lineEdit_password;
    QCheckBox *checkBox_showpswd;

    void setupUi(QWidget *AuthenticationForm)
    {
        if (AuthenticationForm->objectName().isEmpty())
            AuthenticationForm->setObjectName(QString::fromUtf8("AuthenticationForm"));
        AuthenticationForm->setWindowModality(Qt::ApplicationModal);
        AuthenticationForm->setEnabled(true);
        AuthenticationForm->resize(408, 262);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/Resources/Saki-Snowish-Authentication-Lock.ico"), QSize(), QIcon::Normal, QIcon::Off);
        AuthenticationForm->setWindowIcon(icon);
        AuthenticationForm->setAutoFillBackground(true);
        groupBox = new QGroupBox(AuthenticationForm);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        groupBox->setGeometry(QRect(60, 30, 271, 181));
        layoutWidget = new QWidget(groupBox);
        layoutWidget->setObjectName(QString::fromUtf8("layoutWidget"));
        layoutWidget->setGeometry(QRect(60, 120, 158, 25));
        horizontalLayout_2 = new QHBoxLayout(layoutWidget);
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setContentsMargins(11, 11, 11, 11);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        pushButton_signup = new QPushButton(layoutWidget);
        pushButton_signup->setObjectName(QString::fromUtf8("pushButton_signup"));

        horizontalLayout_2->addWidget(pushButton_signup);

        pushButton_signin = new QPushButton(layoutWidget);
        pushButton_signin->setObjectName(QString::fromUtf8("pushButton_signin"));

        horizontalLayout_2->addWidget(pushButton_signin);

        label = new QLabel(groupBox);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(43, 43, 51, 16));
        label_2 = new QLabel(groupBox);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setGeometry(QRect(41, 69, 46, 16));
        layoutWidget1 = new QWidget(groupBox);
        layoutWidget1->setObjectName(QString::fromUtf8("layoutWidget1"));
        layoutWidget1->setGeometry(QRect(93, 43, 135, 71));
        verticalLayout = new QVBoxLayout(layoutWidget1);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        lineEdit_nickname = new QLineEdit(layoutWidget1);
        lineEdit_nickname->setObjectName(QString::fromUtf8("lineEdit_nickname"));
        lineEdit_nickname->setDragEnabled(true);

        verticalLayout->addWidget(lineEdit_nickname);

        lineEdit_password = new QLineEdit(layoutWidget1);
        lineEdit_password->setObjectName(QString::fromUtf8("lineEdit_password"));

        verticalLayout->addWidget(lineEdit_password);

        checkBox_showpswd = new QCheckBox(layoutWidget1);
        checkBox_showpswd->setObjectName(QString::fromUtf8("checkBox_showpswd"));
        checkBox_showpswd->setEnabled(true);
        checkBox_showpswd->setChecked(false);
        checkBox_showpswd->setTristate(false);

        verticalLayout->addWidget(checkBox_showpswd);


        retranslateUi(AuthenticationForm);

        QMetaObject::connectSlotsByName(AuthenticationForm);
    } // setupUi

    void retranslateUi(QWidget *AuthenticationForm)
    {
        AuthenticationForm->setWindowTitle(QApplication::translate("AuthenticationForm", "Authentication", nullptr));
        groupBox->setTitle(QString());
        pushButton_signup->setText(QApplication::translate("AuthenticationForm", "Sign UP", nullptr));
        pushButton_signin->setText(QApplication::translate("AuthenticationForm", "Sign IN", nullptr));
        label->setText(QApplication::translate("AuthenticationForm", "Nicnkname", nullptr));
        label_2->setText(QApplication::translate("AuthenticationForm", "Password", nullptr));
        checkBox_showpswd->setText(QApplication::translate("AuthenticationForm", "show password", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AuthenticationForm: public Ui_AuthenticationForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_AUTHENTICATIONFORM_H
