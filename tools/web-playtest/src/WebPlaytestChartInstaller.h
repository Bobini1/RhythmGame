#pragma once

#include <QString>

namespace web_playtest {

class WebPlaytestChartInstaller final
{
  public:
    [[nodiscard]] static auto install(QString& error) -> QString;
};

}
