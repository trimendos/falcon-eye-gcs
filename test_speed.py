import time

start_time = time.time()

total = 0
for i in range(100000000): # 100 мільйонів ітерацій
    total += i

end_time = time.time()

print(f"Python Result: {total}")
print(f"Python Time: {end_time - start_time:.6f} seconds")