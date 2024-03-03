constexpr const char* tiltColorSuffixes[] = {
    "_red",       // 0 = TILT_COLOR_RED
    "_green",     // 1 = TILT_COLOR_GREEN
    "_black",     // 2 = TILT_COLOR_BLACK
    "_purple",    // 3 = TILT_COLOR_PURPLE
    "_orange",    // 4 = TILT_COLOR_ORANGE
    "_blue",      // 5 = TILT_COLOR_BLUE
    "_yellow",    // 6 = TILT_COLOR_YELLOW
    "_pink",      // 7 = TILT_COLOR_PINK
};



namespace CalibrationKeys {
constexpr auto applyCalibration = "applyCalibration";
constexpr auto tempCorrect = "tempCorrect";
}; // namespace CalibrationKeys


namespace CloudTargetSettings {
constexpr auto cloudTarget = "cloudTarget";
}

namespace FermentrackSettings {
    // TODO - Update these
constexpr auto fermentrackURL = "localTargetURL";
constexpr auto fermentrackPushEvery = "localTargetPushEvery";
}

namespace GoogleSheetsSettings {
constexpr auto scriptsURL = "scriptsURL";
constexpr auto scriptsEmail = "scriptsEmail";
constexpr auto gsheetsPrefix = "sheetName";
}


namespace BrewersFriendSettings {
constexpr auto brewersFriendKey = "brewersFriendKey";
}

namespace BrewfatherSettings {
constexpr auto brewfatherKey = "brewfatherKey";
}

namespace UserTargetSettings {
constexpr auto userTargetURL = "userTargetURL";
}

namespace GrainfatherSettings {
constexpr auto grainfatherURLPrefix = "grainfatherURL";
}

namespace BrewstatusSettings {
constexpr auto brewstatusURL = "brewstatusURL";
constexpr auto brewstatusPushEvery = "brewstatusPushEvery";
}

namespace TaplistioSettings {
constexpr auto taplistioURL = "taplistioURL";
constexpr auto taplistioPushEvery = "taplistioPushEvery";
}

namespace MQTTSettings {
constexpr auto mqttBrokerHost = "mqttBrokerHost";
constexpr auto mqttBrokerPort = "mqttBrokerPort";
constexpr auto mqttPushEvery = "mqttPushEvery";
constexpr auto mqttUsername = "mqttUsername";
constexpr auto mqttPassword = "mqttPassword";
constexpr auto mqttTopic = "mqttTopic";

}

