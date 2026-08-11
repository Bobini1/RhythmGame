#pragma once

#include <QObject>

class Lr2SkinElementNumberState : public QObject
{
    Q_OBJECT
    Q_PROPERTY(
      int dependencyMask READ dependencyMask NOTIFY dependencyMaskChanged)
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)

  public:
    explicit Lr2SkinElementNumberState(QObject* parent = nullptr);

    int dependencyMask() const;
    int revision() const;

    void setDependencyMask(int dependencyMask);
    void refresh();

  signals:
    void dependencyMaskChanged();
    void revisionChanged();

  private:
    int m_dependencyMask = 0;
    int m_revision = 0;
};
