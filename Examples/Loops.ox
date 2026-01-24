// 'while' and 'for' loops in Onyx is the just 'for' loop

fun main() : i32 {
    for var i: i32, i < 10, i += 1 {}   // just 'for' loop

    var i: i32;
    for i < 10 {                        // just 'while' loop
        // doing something...
        i += 1;
    }

    for var i: i32, i < 10, i += 1 {
        if i == 3 {
            continue;
        }
        if i == 5 {
            break;
        }
    }

    return 0;
}
