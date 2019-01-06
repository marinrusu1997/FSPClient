#include "AuthenticationForm.h"
#include "ui_AuthenticationForm.h"
#include "request_manager.h"

#include <QtWidgets/qmessagebox.h>
#include <QtGui/qvalidator.h>

Q_DECLARE_METATYPE(std::string);
Q_DECLARE_METATYPE(std::string_view);

AuthenticationForm::AuthenticationForm(request_manager& req_manager, QWidget *parent)  
	: QDialog(parent), request_manager_(req_manager), isWaitingForAuthenticationResponse(false)
{
	/// Register signal types which can be emitted by this class
	qRegisterMetaType<std::string>();
	qRegisterMetaType<std::string_view>();

	/// Setup UI
	ui = new Ui::AuthenticationForm();
	ui->setupUi(this);

	ui->pushButton_signin->setStyleSheet("border-radius: 4px;");
	ui->pushButton_signup->setStyleSheet("border-radius: 4px;");

	/// Setup Validators
	QRegExp rx("[a-z - 0-9]{0,15}");
	QValidator *validator = new QRegExpValidator(rx, this);
	ui->lineEdit_nickname->setValidator(validator);
	ui->lineEdit_password->setValidator(validator);

	/// Make password invisible
	ui->lineEdit_password->setEchoMode(QLineEdit::Password);
	ui->checkBox_showpswd->setChecked(false);

	/// Disable buttons
	disableButtons(true);

	/// signal GUI -> slot GUI
	QObject::connect(ui->pushButton_signin, &QPushButton::clicked, this, &AuthenticationForm::onSignInButtonPressed);
	QObject::connect(ui->pushButton_signup, &QPushButton::clicked, this, &AuthenticationForm::onSignUpButtonPressed);
	QObject::connect(ui->lineEdit_nickname, &QLineEdit::textChanged, this, &AuthenticationForm::onTextChanged);
	QObject::connect(ui->lineEdit_password, &QLineEdit::textChanged, this, &AuthenticationForm::onTextChanged);
	QObject::connect(ui->checkBox_showpswd, &QCheckBox::clicked, this, &AuthenticationForm::onCheckBoxPswdChanged);
	/// signal GUI -> slot network I/O
	QObject::connect(this, &AuthenticationForm::signin, &request_manager_, &request_manager::on_signin);
	QObject::connect(this, &AuthenticationForm::signup, &request_manager_, &request_manager::on_signup);
	/// signal network I/O -> slot GUI
	QObject::connect(&request_manager_, &request_manager::authentication_status, this, &AuthenticationForm::onAuthenticationStatusReceived);
}

AuthenticationForm::~AuthenticationForm()
{
	delete ui;
}

void AuthenticationForm::disableButtons(bool disable) noexcept
{
	ui->pushButton_signin->setEnabled(!disable);
	ui->pushButton_signup->setEnabled(!disable);
}

void AuthenticationForm::onTextChanged()
{
	disableButtons(!((ui->lineEdit_nickname->text().size() > 0)
		& (ui->lineEdit_password->text().size() > 0)));
}

void AuthenticationForm::onCheckBoxPswdChanged(bool isChecked)
{
	if (isChecked)
	{
		ui->lineEdit_password->setEchoMode(QLineEdit::EchoMode::Normal);
	}
	else
	{
		ui->lineEdit_password->setEchoMode(QLineEdit::Password);
	}
}

void AuthenticationForm::onSignInButtonPressed()
{
	if (isWaitingForAuthenticationResponse)
		return;
	emit signin(ui->lineEdit_nickname->text().toStdString(),
		ui->lineEdit_password->text().toStdString());
	isWaitingForAuthenticationResponse = true;
}

void AuthenticationForm::onSignUpButtonPressed()
{
	if (isWaitingForAuthenticationResponse)
		return;
	emit signup(ui->lineEdit_nickname->text().toStdString(),
		ui->lineEdit_password->text().toStdString());
	isWaitingForAuthenticationResponse = true;
}

void AuthenticationForm::onAuthenticationStatusReceived(const char *status)
{
	if (status == nullptr)
	{
		current_user_logged_ = ui->lineEdit_nickname->text();
		this->accept();
		this->close();
	}
	else
	{
		QMessageBox msgBox("Authentication Failure", status , QMessageBox::Icon::Warning, QMessageBox::Button::Ok, QMessageBox::Button::Cancel, 0);
		msgBox.exec();
		isWaitingForAuthenticationResponse = false;
	}
}
