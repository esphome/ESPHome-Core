#ifndef ESPHOME_CORE_COVER_AUTOMATION_H
#define ESPHOME_CORE_COVER_AUTOMATION_H

#include "esphome/defines.h"

#ifdef USE_COVER

#include "esphome/automation.h"
#include "esphome/cover/cover.h"

ESPHOME_NAMESPACE_BEGIN

namespace cover {

template<typename... Ts> class OpenAction : public Action<Ts...> {
 public:
  explicit OpenAction(Cover *cover) : cover_(cover) {}

  void play(Ts... x) override {
    this->cover_->open();
    this->play_next(x...);
  }

 protected:
  Cover *cover_;
};

template<typename... Ts> class CloseAction : public Action<Ts...> {
 public:
  explicit CloseAction(Cover *cover) : cover_(cover) {}

  void play(Ts... x) override {
    this->cover_->close();
    this->play_next(x...);
  }

 protected:
  Cover *cover_;
};

template<typename... Ts> class StopAction : public Action<Ts...> {
 public:
  explicit StopAction(Cover *cover) : cover_(cover) {}

  void play(Ts... x) override {
    this->cover_->stop();
    this->play_next(x...);
  }

 protected:
  Cover *cover_;
};

template<typename... Ts> class CoverPublishAction : public Action<Ts...> {
 public:
  CoverPublishAction(Cover *cover) : cover_(cover) {}
  void play(Ts... x) override {
    if (this->state_.has_value())
      this->cover_->position = this->state_.value(x...);
    if (this->tilt_.has_value())
      this->cover_->tilt = this->tilt_.value(x...);
    this->cover_->publish_state();
    this->play_next(x...);
  }

  TEMPLATABLE_VALUE(float, state)
  TEMPLATABLE_VALUE(float, tilt)
  // TODO: current_operation

 protected:
  Cover *cover_;
};

template<typename... Ts> class CoverIsOpenCondition : public Condition<Ts...> {
 public:
  CoverIsOpenCondition(Cover *cover) : cover_(cover) {}
  bool check(Ts... x) override { return this->cover_->position == COVER_OPEN; }
 protected:
  Cover *cover_;
};
template<typename... Ts> class CoverIsClosedCondition : public Condition<Ts...> {
 public:
  CoverIsClosedCondition(Cover *cover) : cover_(cover) {}
  bool check(Ts... x) override { return this->cover_->position == COVER_CLOSED; }
 protected:
  Cover *cover_;
};

}  // namespace cover

ESPHOME_NAMESPACE_END

#endif //USE_COVER

#endif //ESPHOME_CORE_COVER_AUTOMATION_H
