#pragma once

#include <QtWidgets/qdialog.h>
#include <string_view>
namespace Ui { class AuthenticationForm; };

class request_manager;

class AuthenticationForm : public QDialog
{
	Q_OBJECT

public:
	AuthenticationForm(request_manager& req_manager, QWidget *parent = Q_NULLPTR);
	~AuthenticationForm();

	auto& current_user() { return current_user_logged_; }
signals:
	void signup(const std::string, const std::string);
	void signin(const std::string, const std::string);
private slots :
	void onSignInButtonPressed();
	void onSignUpButtonPressed();
	void onTextChanged();
	void onCheckBoxPswdChanged(bool);

	void onAuthenticationStatusReceived(const char *status);
private:

	inline void disableButtons(bool) noexcept;

	request_manager&		request_manager_;
	QString					current_user_logged_;
	bool					isWaitingForAuthenticationResponse;
	Ui::AuthenticationForm	*ui;
};
