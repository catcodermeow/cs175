#ifndef RIGTFORM_H
#define RIGTFORM_H

#include <iostream>
#include <cassert>

#include "matrix4.h"
#include "quat.h"

class RigTForm {
  Cvec3 t_; // translation component
  Quat r_;  // rotation component represented as a quaternion

public:
  RigTForm() : t_(0) {
    assert(norm2(Quat(1,0,0,0) - r_) < CS175_EPS2);
  }

  RigTForm(const Cvec3& t, const Quat& r) {
      t_ = t;
      r_ = r;
  }

  explicit RigTForm(const Cvec3& t) {
      t_ = t;
      r_ = Quat(); // no rotation
  }

  explicit RigTForm(const Quat& r) {
      t_ = Cvec3(); // no translation
      r_ = r;
  }

  Cvec3 getTranslation() const {
    return t_;
  }

  Quat getRotation() const {
    return r_;
  }

  RigTForm& setTranslation(const Cvec3& t) {
    t_ = t;
    return *this;
  }

  RigTForm& setRotation(const Quat& r) {
    r_ = r;
    return *this;
  }

  Cvec4 operator * (const Cvec4& a) const {
      // multiply by r but keep the last place of a
      Cvec4 value = Cvec4(r_ * Cvec3(a), a[3]);
      value += Cvec4(t_, 0) * a[-1];
      return value;
  }

  RigTForm operator * (const RigTForm& a) const {
    // resulting rigtform has translation t1 + r1t2
    // and rotation r1r2
      return RigTForm(t_ + r_*a.getTranslation(), r_*a.getRotation());
  }
};

inline RigTForm inv(const RigTForm& tform) {
  // inverted rigtform has translation -r^{-1} t and rotation r^{-1}
    Quat invr_ = inv(tform.getRotation());
    return RigTForm(-(invr_ * tform.getTranslation()), invr_);
}

inline RigTForm transFact(const RigTForm& tform) {
  return RigTForm(tform.getTranslation());
}

inline RigTForm linFact(const RigTForm& tform) {
  return RigTForm(tform.getRotation());
}

inline Matrix4 rigTFormToMatrix(const RigTForm& tform) {
  // order: T * R
    return Matrix4::makeTranslation(tform.getTranslation()) * quatToMatrix(tform.getRotation());
}

#endif
