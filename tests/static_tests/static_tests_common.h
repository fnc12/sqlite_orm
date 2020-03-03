#pragma once

struct User {
    int id;

    const int &getIdByRefConst() const {
        return this->id;
    }

    const int &getIdByRef() {
        return this->id;
    }

    int getIdByValConst() const {
        return this->id;
    }

    void setIdByVal(int id) {
        this->id = id;
    }

    void setIdByConstRef(const int &id) {
        this->id = id;
    }

    void setIdByRef(int &id) {
        this->id = id;
    }
};

struct Object {
    int id;
};

struct Token : Object {};
