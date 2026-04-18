#pragma once

#include <QString>
#include <QVariant>
#include <QList>
#include <QMap>
#include <QMetaType>

namespace gameplay_logic::lr2_skin {

struct Lr2Dst
{
    Q_GADGET
    Q_PROPERTY(int time MEMBER time)
    Q_PROPERTY(int x MEMBER x)
    Q_PROPERTY(int y MEMBER y)
    Q_PROPERTY(int w MEMBER w)
    Q_PROPERTY(int h MEMBER h)
    Q_PROPERTY(int acc MEMBER acc)
    Q_PROPERTY(int a MEMBER a)
    Q_PROPERTY(int r MEMBER r)
    Q_PROPERTY(int g MEMBER g)
    Q_PROPERTY(int b MEMBER b)
    Q_PROPERTY(int blend MEMBER blend)
    Q_PROPERTY(int filter MEMBER filter)
    Q_PROPERTY(int angle MEMBER angle)
    Q_PROPERTY(int center MEMBER center)
    Q_PROPERTY(int loop MEMBER loop)
    Q_PROPERTY(int timer MEMBER timer)
    Q_PROPERTY(int op1 MEMBER op1)
    Q_PROPERTY(int op2 MEMBER op2)
    Q_PROPERTY(int op3 MEMBER op3)
    Q_PROPERTY(int op4 MEMBER op4)
  public:
    int time = 0;
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int acc = 0;
    int a = 255;
    int r = 255;
    int g = 255;
    int b = 255;
    int blend = 0;
    int filter = 0;
    int angle = 0;
    int center = 0;
    int loop = 0;
    int timer = 0;
    int op1 = 0;
    int op2 = 0;
    int op3 = 0;
    int op4 = 0;
};

struct Lr2SrcImage
{
    Q_GADGET
    Q_PROPERTY(int gr MEMBER gr)
    Q_PROPERTY(int x MEMBER x)
    Q_PROPERTY(int y MEMBER y)
    Q_PROPERTY(int w MEMBER w)
    Q_PROPERTY(int h MEMBER h)
    Q_PROPERTY(int div_x MEMBER div_x)
    Q_PROPERTY(int div_y MEMBER div_y)
    Q_PROPERTY(int cycle MEMBER cycle)
    Q_PROPERTY(int timer MEMBER timer)
    Q_PROPERTY(int op1 MEMBER op1)
    Q_PROPERTY(int op2 MEMBER op2)
    Q_PROPERTY(int op3 MEMBER op3)
    Q_PROPERTY(int specialType MEMBER specialType)
    Q_PROPERTY(QString source MEMBER source)
  public:
    enum SpecialType
    {
        None = 0,
        StageFile = 1,
        SolidBlack = 2
    };

    int gr = 0;
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int div_x = 1;
    int div_y = 1;
    int cycle = 0;
    int timer = 0;
    int op1 = 0;
    int op2 = 0;
    int op3 = 0;
    int specialType = None;
    QString source;
};

struct Lr2SrcNumber
{
    Q_GADGET
    Q_PROPERTY(int gr MEMBER gr)
    Q_PROPERTY(int x MEMBER x)
    Q_PROPERTY(int y MEMBER y)
    Q_PROPERTY(int w MEMBER w)
    Q_PROPERTY(int h MEMBER h)
    Q_PROPERTY(int div_x MEMBER div_x)
    Q_PROPERTY(int div_y MEMBER div_y)
    Q_PROPERTY(int cycle MEMBER cycle)
    Q_PROPERTY(int timer MEMBER timer)
    Q_PROPERTY(int num MEMBER num)
    Q_PROPERTY(int align MEMBER align)
    Q_PROPERTY(int keta MEMBER keta)
  public:
    int gr = 0;
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int div_x = 1;
    int div_y = 1;
    int cycle = 0;
    int timer = 0;
    int num = 0;
    int align = 0;
    int keta = 0;
};

struct Lr2SrcText
{
    Q_GADGET
    Q_PROPERTY(int font MEMBER font)
    Q_PROPERTY(int st MEMBER st)
    Q_PROPERTY(int align MEMBER align)
    Q_PROPERTY(int edit MEMBER edit)
    Q_PROPERTY(int panel MEMBER panel)
    Q_PROPERTY(QString fontPath MEMBER fontPath)
    Q_PROPERTY(QString fontFamily MEMBER fontFamily)
    Q_PROPERTY(int fontSize MEMBER fontSize)
    Q_PROPERTY(int fontThickness MEMBER fontThickness)
    Q_PROPERTY(int fontType MEMBER fontType)
    Q_PROPERTY(bool bitmapFont MEMBER bitmapFont)
  public:
    int font = 0;
    int st = 0;
    int align = 0;
    int edit = 0;
    int panel = 0;
    QString fontPath;
    QString fontFamily;
    int fontSize = 0;
    int fontThickness = 0;
    int fontType = 0;
    bool bitmapFont = false;
};

struct Lr2Element
{
    Q_GADGET
    Q_PROPERTY(int type MEMBER type)
    Q_PROPERTY(QVariant src MEMBER src)
    Q_PROPERTY(QVariantList dsts MEMBER dsts)
  public:
    int type = -1; // 0 = image, 1 = number
    QVariant src;
    QVariantList dsts;
};

struct Lr2SkinData
{
    QList<Lr2Element> elements;
    int startInput = 0;
    int sceneTime = 0;
    int fadeOut = 0;
    int skip = 0;
};

class Lr2SkinParser
{
  public:
    static QList<Lr2Element> parse(const QString& path,
                                   const QVariantMap& settingValues = {},
                                   const QVariantList& activeOptions = {});
    static Lr2SkinData parseData(const QString& path,
                                 const QVariantMap& settingValues = {},
                                 const QVariantList& activeOptions = {});
};

auto
resolvePath(const QString& path) -> QString;

} // namespace gameplay_logic::lr2_skin
