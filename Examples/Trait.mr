struct Int {
    var v: i32;
}

trait TAddable {
    pub fun add(other: i32): Int;
}

trait TSubtractable {
    pub fun sub(other: i32): Int;
}

impl TAddable for Int {
    pub fun add(other: i32): Int {
        this = Int { v: this.v + other };
        return this;
    }
}

impl TSubtractable for Int {
    pub fun sub(other: i32): Int {
        this = Int { v: this.v - other };
        return this;
    }
}

impl Int {
    pub fun print(): noth {
        echo this.v;
    }

    pub fun test(): noth {
        this = Int { v: 16 };
    }
}

fun main(): i32 {
    var i: Int = Int { v: 2 };
    i.add(3);
    i.print();
    i.sub(5);
    i.print();
    return 0;
}
