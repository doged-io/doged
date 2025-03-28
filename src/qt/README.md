This directory contains the BitcoinQT graphical user interface (GUI). It uses the cross platform framework [QT](https://www1.qt.io/developers/).

The current precise version for QT 5 is specified in [qt.mk](/depends/packages/qt.mk).

## Compile and run

See build instructions ([macOS](/doc/build-osx.md), [Windows](/doc/build-windows.md), [Unix](/doc/build-unix.md), etc).

To run:

```sh
./src/qt/doge-qt
```

## Files and directories

### forms

Contains [Designer UI](http://doc.qt.io/qt-5.9/designer-using-a-ui-file.html) files. They are created with [Qt Creator](#using-qt-creator-as-ide), but can be edited using any text editor.

### locale

Contains translations. They are periodically updated. The process is described [here](/doc/translation_process.md).

### res

Resources such as the icon.

### test

Tests (see [the unit tests documentation](/doc/unit-tests.md)).

### bitcoingui.(h/cpp)

Represents the main window of the Bitcoin UI.

### \*model.(h/cpp)

The model. When it has a corresponding controller, it generally inherits from  [QAbstractTableModel](http://doc.qt.io/qt-5/qabstracttablemodel.html). Models that are used by controllers as helpers inherit from other QT classes like [QValidator](http://doc.qt.io/qt-5/qvalidator.html).

ClientModel is used by the main application `bitcoingui` and several models like `peertablemodel`.

### \*page.(h/cpp)

A controller. `:NAMEpage.cpp` generally includes `:NAMEmodel.h` and `forms/:NAME.page.ui` with a similar `:NAME`.

### \*dialog.(h/cpp)

Various dialogs, e.g. to open a URL. Inherit from [QDialog](http://doc.qt.io/qt-5.5/qdialog.html).

### paymentserver.(h/cpp)

Used to process BIP21 and BIP70 payment URI / requests. Also handles URI based application switching (e.g. when following a bitcoincash:... link from a browser).

### walletview.(h/cpp)

Represents the view to a single wallet.

### Other .h/cpp files

* UI elements like BitcoinAmountField, which inherit from QWidget.
* `bitcoinstrings.cpp`: automatically generated
* `bitcoinunits.(h/cpp)`: BCH / mBCH / etc handling
* `callback.h`
* `guiconstants.h`: UI colors, app name, etc
* `guiutil.h`: several helper functions
* `macdockiconhandler.(h/mm)`: macOS dock icon handler
* `macnotificationhandler.(h/mm)`: display notifications in macOS
* `macos_appnap.(h/mm)`: disable appnap during sync in macOS

## Contribute

See [CONTRIBUTING.md](/CONTRIBUTING.md) for general guidelines. Specifically for QT:

* don't change `local/bitcoin_en.ts`; this happens [automatically](/doc/translation_process.md#writing-code-with-translations)

## Using Qt Creator as IDE

You can use Qt Creator as an IDE. This is especially useful if you want to change
the UI layout.

Download and install the community edition of [Qt Creator](https://www.qt.io/download/).
Uncheck everything except Qt Creator during the installation process.

Instructions for macOS:

1. Make sure you installed everything through Homebrew mentioned in the [macOS build instructions](/doc/build-osx.md)
2. Use `cmake` with the `-DCMAKE_BUILD_TYPE=Debug` flag
3. In Qt Creator do "New Project" -> Import Project -> Import Existing Project
4. Enter "doge-qt" as project name, enter src/qt as location
5. Leave the file selection as it is
6. Confirm the "summary page"
7. In the "Projects" tab select "Manage Kits..."
8. Select the default "Desktop" kit and select "Clang (x86 64bit in /usr/bin)" as compiler
9. Select LLDB as debugger (you might need to set the path to your installation)
10. Start debugging with Qt Creator (you might need to the executable to "doge-qt" under "Run", which is where you can also add command line arguments)
