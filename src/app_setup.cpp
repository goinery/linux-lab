#include "app_setup.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QFont>
#include <QFontDatabase>

#include "constants.h"
#include "unified_flow_window.h"

namespace {

const QString kBundledFontPath = QStringLiteral(":/fonts/font.ttf");

QString loadBundledFontFamily() {
    const int fontId = QFontDatabase::addApplicationFont(kBundledFontPath);
    if (fontId < 0) {
        qCritical() << "Unable to load required bundled font" << kBundledFontPath;
        return {};
    }

    const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
    if (families.isEmpty()) {
        qCritical() << "Required bundled font has no family metadata";
        return {};
    }

    qInfo() << "Loaded bundled font:" << kBundledFontPath
            << "family:" << families.constFirst();
    return families.constFirst();
}

} // namespace

void configureInputMethodEnvironment() {
    if (qEnvironmentVariableIsEmpty("QT_IM_MODULE")) {
        const QByteArray gtkIm = qgetenv("GTK_IM_MODULE");
        const QByteArray xModifiers = qgetenv("XMODIFIERS");

        if (gtkIm.contains("ibus") || xModifiers.contains("@im=ibus")) {
            qputenv("QT_IM_MODULE", "ibus");
        } else if (gtkIm.contains("fcitx") || xModifiers.contains("@im=fcitx")) {
            qputenv("QT_IM_MODULE", "fcitx");
        }
    }

    const QByteArray qtImModule = qgetenv("QT_IM_MODULE");
    if (qEnvironmentVariableIsEmpty("XMODIFIERS")) {
        if (qtImModule == "ibus") {
            qputenv("XMODIFIERS", "@im=ibus");
        } else if (qtImModule.startsWith("fcitx")) {
            qputenv("XMODIFIERS", "@im=fcitx");
        }
    }

    qInfo() << "IME env: QT_IM_MODULE=" << qgetenv("QT_IM_MODULE")
            << "XMODIFIERS=" << qgetenv("XMODIFIERS")
            << "GTK_IM_MODULE=" << qgetenv("GTK_IM_MODULE");
}

bool configureApplicationFont(QApplication &app) {
    const QString fontFamily = loadBundledFontFamily();
    if (fontFamily.isEmpty()) {
        return false;
    }
    QFont appFont = app.font();
    appFont.setStyleHint(QFont::SansSerif);
    appFont.setPointSize(12);
    appFont.setFamily(fontFamily);
    app.setFont(appFont);
    return true;
}

int runApp(int argc, char *argv[]) {
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    configureInputMethodEnvironment();

    QApplication app(argc, argv);
    QApplication::setApplicationName(Constants::APP_NAME);
    QApplication::setApplicationVersion(Constants::APP_VERSION);
    if (!configureApplicationFont(app)) {
        return 1;
    }

    QCommandLineParser parser;
    parser.setApplicationDescription("ChatRoom");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({{"H", "host"}, "Server host (default: 127.0.0.1)",
                       "host", Constants::DEFAULT_HOST});
    parser.addOption({{"p", "port"}, "Server port (default: 8888)", "port", "8888"});
    parser.addOption({{"s", "server"}, "Preselect server mode on startup page"});
    parser.process(app);

    const QString defaultHost = parser.value("host");
    bool portOk = false;
    const uint parsedPort = parser.value("port").toUInt(&portOk);
    const quint16 defaultPort = portOk && parsedPort > 0 && parsedPort <= 65535
        ? quint16(parsedPort)
        : Constants::DEFAULT_PORT;
    const bool defaultServerMode = parser.isSet("server");

    UnifiedFlowWindow window(defaultHost, defaultPort, defaultServerMode);
    window.show();

    return app.exec();
}
