class ControlBlock {
  constructor(value, destructor) {
    this.value = value;
    this.count = 1;
    this.destructor = destructor;
    this.released = false;
  }

  retain() {
    if (this.released) {
      throw new Error("Cannot retain a released pointer");
    }
    this.count++;
  }

  release() {
    if (this.released) return;

    this.count--;

    if (this.count === 0) {
      this.released = true;
      const value = this.value;
      this.value = undefined;
      if (this.destructor) this.destructor(value);
    }
  }
}

class SmartPointer {
  constructor(value, destructor = null, controlBlock = null) {
    if (controlBlock) {
      this._control = controlBlock;
      this._control.retain();
    } else {
      this._control = value === null || value === undefined
        ? null
        : new ControlBlock(value, destructor);
    }

    this._released = false;
  }

  static make(value, destructor = null) {
    return new SmartPointer(value, destructor);
  }

  clone() {
    this._ensureValid();
    return new SmartPointer(undefined, undefined, this._control);
  }

  get() {
    this._ensureValid();
    return this._control.value;
  }

  useCount() {
    return this._control && !this._control.released
      ? this._control.count
      : 0;
  }

  reset(value = null, destructor = null) {
    this.release();

    if (value !== null && value !== undefined) {
      this._control = new ControlBlock(value, destructor);
      this._released = false;
    }
  }

  release() {
    if (this._released) return;

    if (this._control) {
      this._control.release();
      this._control = null;
    }

    this._released = true;
  }

  isEmpty() {
    return this._released || !this._control || this._control.released;
  }

  _ensureValid() {
    if (this.isEmpty()) {
      throw new Error("Dereferencing an empty or released SmartPointer");
    }
  }
}

module.exports = { SmartPointer };
