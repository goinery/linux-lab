#ifndef APP_SETUP_H
#define APP_SETUP_H

class QApplication;

void configureInputMethodEnvironment();
bool configureApplicationFont(QApplication &app);
int runApp(int argc, char *argv[]);

#endif
