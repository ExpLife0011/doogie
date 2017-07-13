#include "dev_tools_dock.h"

DevToolsDock::DevToolsDock(Cef* cef,
                           BrowserStack *browser_stack,
                           QWidget *parent)
    : QDockWidget("Dev Tools", parent),
      cef_(cef),
      browser_stack_(browser_stack) {
  setFeatures(QDockWidget::AllDockWidgetFeatures);

  connect(browser_stack_, &BrowserStack::BrowserChanged,
          this, &DevToolsDock::BrowserChanged);

  auto not_there_label = new QLabel("No browser open");
  not_there_label->setAlignment(Qt::AlignCenter);
  not_there_label->autoFillBackground();

  auto open_dev_tools_btn = new QPushButton("Click to open dev tools");
  connect(open_dev_tools_btn, &QPushButton::clicked, [this](bool) {
    ShowDevTools(browser_stack_->CurrentBrowser());
  });
  auto open_layout = new QGridLayout();
  open_layout->setColumnStretch(0, 1);
  open_layout->setRowStretch(0, 1);
  open_layout->addWidget(open_dev_tools_btn, 1, 1);
  open_layout->setColumnStretch(2, 1);
  open_layout->setRowStretch(2, 1);
  auto open_widg = new QWidget();
  open_widg->setLayout(open_layout);

  tools_stack_ = new QStackedWidget();
  tools_stack_->addWidget(not_there_label);
  tools_stack_->addWidget(open_widg);
  setWidget(tools_stack_);

  // Go ahead and invoke as though the browser changed
  BrowserChanged(browser_stack->CurrentBrowser());
}

void DevToolsDock::BrowserChanged(BrowserWidget* browser) {
  if (!browser) {
    tools_stack_->setCurrentIndex(0);
  } else {
    auto tools_widg = tools_widgets_[browser];
    if (tools_widg) {
      tools_stack_->setCurrentWidget(tools_widg);
    } else {
      tools_stack_->setCurrentIndex(1);
    }
  }
}

void DevToolsDock::ShowDevTools(BrowserWidget* browser) {
  auto widg = new CefBaseWidget(cef_, tools_stack_);
  tools_stack_->addWidget(widg);
  tools_widgets_[browser] = widg;
  // We make "this" the context of the connections so we can
  //  disconnect later
  // Need to show that close button
  connect(browser, &BrowserWidget::DevToolsLoadComplete, this,
          [this, browser]() {
    // TODO: put a unit test around this highly-volatile code
    QString js =
        "Components.dockController._closeButton.setVisible(true);\n"
        "Components.dockController._closeButton.addEventListener(\n"
        "  UI.ToolbarButton.Events.Click,\n"
        "  () => window.close()\n"
        ");\n";
    browser->ExecDevToolsJs(js);
  });
  // On close, remove it
  connect(browser, &BrowserWidget::DevToolsClosed,
          this, [this, browser]() { DevToolsClosed(browser); });
  // On destroy, remove it
  connect(browser, &BrowserWidget::destroyed,
          this, [this, browser]() { DevToolsClosed(browser); });
  browser->ShowDevTools(widg);
  tools_stack_->setCurrentWidget(widg);
}

void DevToolsDock::DevToolsClosed(BrowserWidget *browser) {
  auto widg = tools_widgets_[browser];
  if (widg) {
    tools_widgets_.remove(browser);
    disconnect(browser, &BrowserWidget::DevToolsLoadComplete, this, nullptr);
    disconnect(browser, &BrowserWidget::DevToolsClosed, this, nullptr);
    disconnect(browser, &BrowserWidget::destroyed, this, nullptr);
    tools_stack_->removeWidget(widg);
    delete widg;
  }
}