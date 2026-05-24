#include "app_setup.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QScreen>
#include <QWindow>

#include "constants.h"
#include "cursor_manager.h"
#include "unified_flow_window.h"

static QStringList preferredCjkFamilies() {
    return {
        "LXGW WenKai",
        "Huawei Sans",
        "Noto Sans CJK SC",
        "Noto Sans SC",
        "WenQuanYi Micro Hei",
        "WenQuanYi Zen Hei",
        "Microsoft YaHei",
        "PingFang SC",
        "Source Han Sans CN"
    };
}

static QString pickPreferredFamily(const QStringList &families) {
    const QStringList preferred = preferredCjkFamilies();
    for (const QString &candidate : preferred) {
        for (const QString &family : families) {
            if (candidate.compare(family, Qt::CaseInsensitive) == 0) {
                return family;
            }
        }
    }

    return families.isEmpty() ? QString() : families.first();
}

QString loadEmbeddedFontFamily() {
    QDir embeddedFontDir(":/fonts");
    if (!embeddedFontDir.exists()) {
        return QString();
    }

    const QStringList fontFiles = embeddedFontDir.entryList(
        {"*.ttf", "*.ttc", "*.otf"}, QDir::Files, QDir::Name);

    QStringList loadedFamilies;
    for (const QString &fontFile : fontFiles) {
        const QString resourcePath = QString(":/fonts/%1").arg(fontFile);
        const int fontId = QFontDatabase::addApplicationFont(resourcePath);
        if (fontId < 0) {
            qWarning() << "Failed to load embedded font:" << resourcePath;
            continue;
        }

        const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
        if (families.isEmpty()) {
            qWarning() << "Embedded font has no family info:" << resourcePath;
            continue;
        }

        loadedFamilies.append(families);
        qInfo() << "Loaded embedded font:" << resourcePath << "families:" << families;
    }

    const QString selectedFamily = pickPreferredFamily(loadedFamilies);
    if (!selectedFamily.isEmpty()) {
        qInfo() << "Selected embedded font family:" << selectedFamily;
    }

    return selectedFamily;
}

QString chooseInstalledFontFamily() {
    const QStringList preferredFamilies = preferredCjkFamilies();

    QFontDatabase fontDatabase;
    const QStringList availableFamilies = fontDatabase.families();

    for (const QString &candidate : preferredFamilies) {
        if (availableFamilies.contains(candidate, Qt::CaseInsensitive)) {
            return candidate;
        }
    }

    return QString();
}

void configureInputMethodEnvironment() {
    if (qEnvironmentVariableIsEmpty("QT_IM_MODULE")) {
        const QByteArray gtkIm = qgetenv("GTK_IM_MODULE");
        const QByteArray xModifiers = qgetenv("XMODIFIERS");

        if (gtkIm.contains("ibus") || xModifiers.contains("@im=ibus")) {
            qputenv("QT_IM_MODULE", "ibus");
        } else if (gtkIm.contains("fcitx") || xModifiers.contains("@im=fcitx")) {
            qputenv("QT_IM_MODULE", "fcitx");
        } else {
            qputenv("QT_IM_MODULE", "ibus");
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

QString configureClientFont(QApplication &app) {
    QString fontFamily = loadEmbeddedFontFamily();
    QString fontSource;

    if (!fontFamily.isEmpty()) {
        fontSource = "embedded-resource";
    } else {
        fontFamily = chooseInstalledFontFamily();
        if (!fontFamily.isEmpty()) {
            fontSource = "system-font";
        }
    }

    QFont appFont;
    appFont.setStyleHint(QFont::SansSerif);
    appFont.setPointSize(12);
    if (!fontFamily.isEmpty()) {
        appFont.setFamily(fontFamily);
    }

    app.setFont(appFont);

    if (fontFamily.isEmpty()) {
        qWarning() << "No preferred CJK font found. UI may show tofu squares."
                   << "Install fonts-noto-cjk or bundle a CJK font in resources/fonts/.";
    } else {
        qInfo() << "UI font configured from" << fontSource << "family:" << fontFamily;
    }

    return fontFamily;
}

int runApp(int argc, char *argv[]) {
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    configureInputMethodEnvironment();

    QApplication app(argc, argv);
    QApplication::setApplicationName(Constants::APP_NAME);
    QApplication::setApplicationVersion(Constants::APP_VERSION);
    const QString selectedFontFamily = configureClientFont(app);

    // 初始化自定义光标（从 PNG 资源加载，回退到 Qt 内置光标）
    CursorManager::instance().initialize();

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
    quint16 defaultPort = quint16(parser.value("port").toUInt());
    if (defaultPort == 0) defaultPort = Constants::DEFAULT_PORT;
    const bool defaultServerMode = parser.isSet("server");

    Q_UNUSED(selectedFontFamily)

    UnifiedFlowWindow window(defaultHost, defaultPort, defaultServerMode);
    window.show();
    if (window.windowHandle()) {
        window.windowHandle()->setMinimumSize(QSize(800, 540));
    }

    return app.exec();
}
