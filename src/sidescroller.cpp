void UpdateAndRender() {
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);    
    
    LARGE_INTEGER start, end;
    QueryPerformanceCounter(&start);
    
    for (;;) {
        QueryPerformanceCounter(&end);
        
        int64_t ticks = end.QuadPart - start.QuadPart;
        double seconds_elapsed = ticks / (double)frequency.QuadPart;
        double ms_elapsed = seconds_elapsed * 1000;
        
        if (ms_elapsed >= 5) {
            break;
        }
    }
}
