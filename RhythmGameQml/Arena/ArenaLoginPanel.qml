import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import RhythmGameQml

FocusScope {
    id: root

    required property Profile profile
    required property bool admissionAllowed
    property bool actionRequired: false

    signal loginSubmitted(string email, string password)

    implicitHeight: panel.implicitHeight
    implicitWidth: panel.implicitWidth
    enabled: root.admissionAllowed

    function clearCredentials() : void {
        emailField.clear();
        passwordField.clear();
    }

    function submit() : void {
        if (!root.admissionAllowed
                || profile.loginState === Profile.LoggingIn
                || emailField.text.length === 0
                || passwordField.text.length === 0) {
            return;
        }
        const email = emailField.text;
        const password = passwordField.text;
        passwordField.clear();
        loginSubmitted(email, password);
    }

    onProfileChanged: clearCredentials()
    onAdmissionAllowedChanged: {
        if (!admissionAllowed) {
            clearCredentials();
        }
    }

    Frame {
        id: panel

        anchors.fill: parent
        implicitHeight: content.implicitHeight + topPadding + bottomPadding
        implicitWidth: content.implicitWidth + leftPadding + rightPadding

        ColumnLayout {
            id: content

            anchors.fill: parent
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: root.actionRequired
                    ? qsTr("Log in to continue with the selected room.")
                    : qsTr("Log in to create or join rooms.")
                wrapMode: Text.Wrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                TextField {
                    id: emailField

                    Accessible.name: qsTr("Arena account email")
                    KeyNavigation.tab: passwordField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Email")
                    selectByMouse: true

                    onAccepted: passwordField.forceActiveFocus()
                }

                TextField {
                    id: passwordField

                    Accessible.name: qsTr("Arena account password")
                    KeyNavigation.tab: loginButton
                    Layout.fillWidth: true
                    echoMode: TextInput.Password
                    inputMethodHints: Qt.ImhHiddenText | Qt.ImhSensitiveData | Qt.ImhNoPredictiveText
                    placeholderText: qsTr("Password")
                    selectByMouse: true

                    onAccepted: root.submit()
                }

                Button {
                    id: loginButton

                    enabled: root.profile.loginState !== Profile.LoggingIn
                        && emailField.text.length > 0
                        && passwordField.text.length > 0
                    text: qsTr("Login")

                    onClicked: root.submit()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                visible: root.profile.loginState === Profile.LoggingIn
                    || root.profile.loginState === Profile.LoginFailed

                BusyIndicator {
                    Layout.preferredHeight: 28
                    Layout.preferredWidth: 28
                    running: visible && root.visible
                    visible: root.profile.loginState === Profile.LoggingIn
                }

                Label {
                    Layout.fillWidth: true
                    color: root.profile.loginState === Profile.LoginFailed
                        ? palette.accent
                        : palette.text
                    text: root.profile.loginState === Profile.LoginFailed
                        ? qsTr("Login failed. Check your email and password.")
                        : qsTr("Logging in...")
                    wrapMode: Text.Wrap
                }
            }
        }
    }
}
