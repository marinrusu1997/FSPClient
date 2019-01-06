/********************************************************************************
** Form generated from reading UI file 'MainWindow.ui'
**
** Created by: Qt User Interface Compiler version 5.12.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindowForm
{
public:
    QAction *actionDelete;
    QAction *actionLog_Out;
    QAction *actionDownload;
    QAction *actionMy_Downloads;
    QAction *actionMy_Uploads;
    QWidget *centralWidget;
    QGridLayout *gridLayout;
    QTreeWidget *users_filesTW;
    QMenuBar *menuBar;
    QMenu *menuAccount;
    QMenu *menuDownloads;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *MainWindowForm)
    {
        if (MainWindowForm->objectName().isEmpty())
            MainWindowForm->setObjectName(QString::fromUtf8("MainWindowForm"));
        MainWindowForm->resize(600, 400);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/Resources/Hopstarter-Sleek-Xp-Basic-Files.ico"), QSize(), QIcon::Normal, QIcon::Off);
        MainWindowForm->setWindowIcon(icon);
        actionDelete = new QAction(MainWindowForm);
        actionDelete->setObjectName(QString::fromUtf8("actionDelete"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/icons/Resources/Gakuseisean-Ivista-2-Misc-Delete-Database.ico"), QSize(), QIcon::Normal, QIcon::Off);
        actionDelete->setIcon(icon1);
        actionLog_Out = new QAction(MainWindowForm);
        actionLog_Out->setObjectName(QString::fromUtf8("actionLog_Out"));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/icons/Resources/Visualpharm-Must-Have-Log-Out.ico"), QSize(), QIcon::Normal, QIcon::Off);
        actionLog_Out->setIcon(icon2);
        actionDownload = new QAction(MainWindowForm);
        actionDownload->setObjectName(QString::fromUtf8("actionDownload"));
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/icons/Resources/iconfinder_Download-Computer_379337.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionDownload->setIcon(icon3);
        actionMy_Downloads = new QAction(MainWindowForm);
        actionMy_Downloads->setObjectName(QString::fromUtf8("actionMy_Downloads"));
        actionMy_Uploads = new QAction(MainWindowForm);
        actionMy_Uploads->setObjectName(QString::fromUtf8("actionMy_Uploads"));
        centralWidget = new QWidget(MainWindowForm);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        gridLayout = new QGridLayout(centralWidget);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        users_filesTW = new QTreeWidget(centralWidget);
        QTreeWidgetItem *__qtreewidgetitem = new QTreeWidgetItem();
        __qtreewidgetitem->setText(0, QString::fromUtf8("1"));
        users_filesTW->setHeaderItem(__qtreewidgetitem);
        users_filesTW->setObjectName(QString::fromUtf8("users_filesTW"));
        users_filesTW->header()->setVisible(false);

        gridLayout->addWidget(users_filesTW, 0, 0, 1, 1);

        MainWindowForm->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(MainWindowForm);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 600, 21));
        menuAccount = new QMenu(menuBar);
        menuAccount->setObjectName(QString::fromUtf8("menuAccount"));
        menuDownloads = new QMenu(menuBar);
        menuDownloads->setObjectName(QString::fromUtf8("menuDownloads"));
        MainWindowForm->setMenuBar(menuBar);
        statusBar = new QStatusBar(MainWindowForm);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        MainWindowForm->setStatusBar(statusBar);

        menuBar->addAction(menuAccount->menuAction());
        menuBar->addAction(menuDownloads->menuAction());
        menuAccount->addAction(actionDelete);
        menuAccount->addAction(actionLog_Out);
        menuDownloads->addAction(actionMy_Downloads);
        menuDownloads->addAction(actionMy_Uploads);

        retranslateUi(MainWindowForm);

        QMetaObject::connectSlotsByName(MainWindowForm);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindowForm)
    {
        MainWindowForm->setWindowTitle(QApplication::translate("MainWindowForm", "File Sharing Protocol", nullptr));
        actionDelete->setText(QApplication::translate("MainWindowForm", "Delete", nullptr));
#ifndef QT_NO_TOOLTIP
        actionDelete->setToolTip(QApplication::translate("MainWindowForm", "Delete you account", nullptr));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_SHORTCUT
        actionDelete->setShortcut(QApplication::translate("MainWindowForm", "Alt+D", nullptr));
#endif // QT_NO_SHORTCUT
        actionLog_Out->setText(QApplication::translate("MainWindowForm", "Log Out", nullptr));
#ifndef QT_NO_SHORTCUT
        actionLog_Out->setShortcut(QApplication::translate("MainWindowForm", "Ctrl+Alt+\303\223", nullptr));
#endif // QT_NO_SHORTCUT
        actionDownload->setText(QApplication::translate("MainWindowForm", "Download", nullptr));
#ifndef QT_NO_TOOLTIP
        actionDownload->setToolTip(QApplication::translate("MainWindowForm", "Download File of this User", nullptr));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_SHORTCUT
        actionDownload->setShortcut(QApplication::translate("MainWindowForm", "Ctrl+D", nullptr));
#endif // QT_NO_SHORTCUT
        actionMy_Downloads->setText(QApplication::translate("MainWindowForm", "My Downloads", nullptr));
        actionMy_Uploads->setText(QApplication::translate("MainWindowForm", "My Uploads", nullptr));
        menuAccount->setTitle(QApplication::translate("MainWindowForm", "Account", nullptr));
        menuDownloads->setTitle(QApplication::translate("MainWindowForm", "Downloads", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindowForm: public Ui_MainWindowForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
