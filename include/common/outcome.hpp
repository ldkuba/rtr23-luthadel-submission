#pragma once

class Outcome {
  protected:
    Outcome(const bool outcome_value) : _outcome_state(outcome_value) {}

  private:
    bool _outcome_state = false;

  public:
    bool succeeded() const { return _outcome_state == true; }
    bool failed() const { return _outcome_state == false; }

    const static Outcome Successful;
    const static Outcome Failed;
};

inline const Outcome Outcome::Successful { true };
inline const Outcome Outcome::Failed { false };