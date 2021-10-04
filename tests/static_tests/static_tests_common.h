#pragma once

struct User {
    int id;

    const int& getIdByRefConst() const {
        return this->id;
    }

    const int& getIdByRef() {
        return this->id;
    }

    int getIdByValConst() const {
        return this->id;
    }

    void setIdByVal(int id_) {
        this->id = id_;
    }

    void setIdByConstRef(const int& id_) {
        this->id = id_;
    }

    void setIdByRef(int& id_) {
        this->id = id_;
    }
};

struct Object {
    int id;
};

struct Token : Object {};
