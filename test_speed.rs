use std::time::Instant;

fn main() {
    // Фіксуємо час старту
    let start = Instant::now();

    let mut total: u64 = 0;
    for i in 0..100_000_000 { // 100 мільйонів ітерацій
        total += i;
    }

    // Фіксуємо час фінішу
    let duration = start.elapsed();

    println!("Rust Result: {}", total);
    println!("Rust Time: {:?}", duration);
}