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

template<typename... Ts> class ControlAction : public Action<Ts...> {
 public:
  explicit ControlAction(Cover *cover) : cover_(cover) {}

  void play(Ts... x) override {
    auto call = this->cover_->make_call();
    if (this->stop_.has_value())
      call.set_stop(this->stop_.value(x...));
    if (this->position_.has_value())
      call.set_position(this->position_.value(x...));
    if (this->tilt_.has_value())
      call.set_tilt(this->tilt_.value(x...));
    call.perform();
    this->play_next(x...);
  }

  TEMPLATABLE_VALUE(bool, stop)
  TEMPLATABLE_VALUE(float, position)
  TEMPLATABLE_VALUE(float, tilt)

 protected:
  Cover *cover_;
};

template<typename... Ts> class CoverPublishAction : public Action<Ts...> {
 public:
  CoverPublishAction(Cover *cover) : cover_(cover) {}
  void play(Ts... x) override {
    if (this->position_.has_value())
      this->cover_->position = this->position_.value(x...);
    if (this->tilt_.has_value())
      this->cover_->tilt = this->tilt_.value(x...);
    if (this->current_operation_.has_value())
      this->cover_->current_operation = this->current_operation_.value(x...);
    this->cover_->publish_state();
    this->play_next(x...);
  }

  TEMPLATABLE_VALUE(float, position)
  TEMPLATABLE_VALUE(float, tilt)
  TEMPLATABLE_VALUE(CoverOperation, current_operation)

 protected:
  Cover *cover_;
};

template<typename... Ts> class CoverIsOpenCondition : public Condition<Ts...> {
 public:
  CoverIsOpenCondition(Cover *cover) : cover_(cover) {}
  bool check(Ts... x) override { return this->cover_->is_fully_open(); }

 protected:
  Cover *cover_;
};
template<typename... Ts> class CoverIsClosedCondition : public Condition<Ts...> {
 public:
  CoverIsClosedCondition(Cover *cover) : cover_(cover) {}
  bool check(Ts... x) override { return this->cover_->is_fully_closed(); }

 protected:
  Cover *cover_;
};

}  // namespace cover

ESPHOME_NAMESPACE_END

#endif  // USE_COVER

#endif  // ESPHOME_CORE_COVER_AUTOMATION_H
