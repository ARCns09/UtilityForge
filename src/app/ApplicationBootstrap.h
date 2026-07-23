#pragma once

class QApplication;

namespace utilityforge::app {

class ApplicationBootstrap final
{
public:
    [[nodiscard]] int run(QApplication &application) const;
};

} // namespace utilityforge::app
