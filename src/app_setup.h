#ifndef APP_SETUP_H
#define APP_SETUP_H

class QString;
class QApplication;

QString loadEmbeddedFontFamily();
QString chooseInstalledFontFamily();
void configureInputMethodEnvironment();
QString configureClientFont(QApplication &app);
int runApp(int argc, char *argv[]);

#endif
