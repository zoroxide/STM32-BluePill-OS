int16_t sin100(int16_t angle) {
    /* Normalize to 0-359 */
    while (angle < 0)   { angle += 360; }
    while (angle >= 360) { angle -= 360; }

    /* Use symmetry to reduce to 0-90 */
    int16_t sign = 1;
    if (angle > 180) { angle -= 180; sign = -1; }
    if (angle > 90)  { angle = 180 - angle; }

    /* Parabolic approximation: sin(x) ~ 4x(180-x) / (40500 - x(180-x)) */
    int32_t x = angle;
    int32_t p = x * (180 - x);
    int32_t result = (400 * p) / (40500 - p);

    return (int16_t)(result * sign);
}

int16_t cos100(int16_t angle) {
    return sin100(angle + 90);
}