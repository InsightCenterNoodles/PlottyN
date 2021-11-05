#include "plotty.h"

#include <QCoreApplication>

#include <QCommandLineParser>
#include <QLoggingCategory>


int main(int argc, char* argv[]) {
    auto app = QCoreApplication(argc, argv);

    QCoreApplication::setApplicationName("PlottyN");
    QCoreApplication::setApplicationVersion("0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription("3D Plotting Tool");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption debug_option(
        "d", QCoreApplication::translate("main", "Debug output"));
    parser.addOption(debug_option);

    parser.process(app);

    bool use_debug = parser.isSet(debug_option);

    if (!use_debug) { QLoggingCategory::setFilterRules("*.debug=false"); }

    Plotty plotty(50000);

    return app.exec();
}
