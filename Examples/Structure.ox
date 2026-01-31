// simple structure

struct Human {
    pub var age: i32;   // public field -> you can access of this field
    var height: f32;    // private field -> you cannot access of this field
}

fun main(): i32 {
    var human: Human = Human { age: 16, height: 1.75f };  // it's me :)
    human.age = 16;
    return 0;
}
