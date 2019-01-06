#include "MainWindow.h"
#include "AuthenticationForm.h"
#include "network_thread.h"
#include "ConfigSettings.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{

	try {
		/// Instantiate Application context
		QApplication a(argc, argv);

		int exit_code = 0;
		persistency::ConfigSettings settings;

		/// Start network thread
		network_thread network_thread;

		while (true) 
		{	
			/// Create instance of Main Window
			MainWindow m(network_thread.context());
			m.show();

			/// Make new connection to server
			m.Connection().start(settings.try_get(persistency::keys::client::SERV_ADDR_IPV4, {"192.168.128.88", "192.168.1.102", "127.0.0.1" }),
				settings.try_get(persistency::keys::common::SERV_PORT, { "8070" }).front());

			/// Create instance of authentication window
			if (auto&& authForm = AuthenticationForm(m.Connection().RequestManager()); QDialog::Accepted != authForm.exec())
			{
				exit_code = EXIT_FAILURE;
				break;
			}
			else 
			{
				m.setWindowTitle(_STD move(authForm.current_user()));
				m.send_directory_to_server();
			}

			exit_code = a.exec();
		}

		return exit_code;
	}
	catch (std::exception const& e) {
		QMessageLogger().fatal(e.what());
		return EXIT_FAILURE;
	}
}
