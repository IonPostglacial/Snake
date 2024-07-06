#include <stdint.h>
#include <stdbool.h>

extern void canvas_set_fill_style(uint32_t color);
extern void canvas_fill_rect(int32_t x, int32_t y, int32_t width, int32_t height);
extern void canvas_fill(void);
extern void snake_score_changed(int32_t score);
extern void snake_step_period_updated(int32_t period);
extern void snake_game_over(void);
extern int32_t js_random(int32_t max);

#define COLOR_BACKGROUND 0x00000000
#define COLOR_SNAKE 0x00ff00
#define COLOR_APPLE 0xff0000
#define CELL_SIZE 10
#define GRID_WIDTH 40
#define GRID_HEIGHT 40

enum Direction
{
    DIRECTION_UP,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_RIGHT
};
enum KeyCode
{
    KEY_CODE_ARROW_UP,
    KEY_CODE_ARROW_DOWN,
    KEY_CODE_ARROW_LEFT,
    KEY_CODE_ARROW_RIGHT
};

bool direction_is_opposite(enum Direction dir, enum Direction other)
{
    return dir == DIRECTION_UP && other == DIRECTION_DOWN ||
           dir == DIRECTION_DOWN && other == DIRECTION_UP ||
           dir == DIRECTION_LEFT && other == DIRECTION_RIGHT ||
           dir == DIRECTION_RIGHT && other == DIRECTION_LEFT;
}

struct Position
{
    int32_t x;
    int32_t y;
};

bool position_equals(struct Position pos, struct Position other)
{
    return pos.x == other.x && pos.y == other.y;
}

struct Position position_moved(struct Position pos, enum Direction direction)
{
    switch (direction) {
    case DIRECTION_UP:
        pos.y--;
        break;
    case DIRECTION_DOWN:
        pos.y++;
        break;
    case DIRECTION_LEFT:
        pos.x--;
        break;
    case DIRECTION_RIGHT:
        pos.x++;
        break;
    }
    return pos;
}

struct Snake
{
    struct Position segments[GRID_WIDTH * GRID_HEIGHT];
    int32_t length;
    int32_t head_index;
    enum Direction direction;
};

struct Position snake_head_position(struct Snake *snake)
{
    return snake->segments[snake->head_index];
}

struct Position snake_next_head_position(struct Snake *snake)
{
    return position_moved(snake->segments[snake->head_index], snake->direction);
}

bool snake_eats_himself(struct Snake *snake)
{
    for (int32_t i = 0; i < snake->length; i++) {
        if (i == snake->head_index) {
            continue;
        }
        if (position_equals(snake_head_position(snake), snake->segments[i])) {
            return true;
        }
    }
    return false;
}

bool snake_is_out_of_bounds(struct Snake *snake, int32_t width, int32_t height)
{
    struct Position headPosition = snake_head_position(snake);
    return headPosition.x < 0 ||
           headPosition.x >= width ||
           headPosition.y < 0 ||
           headPosition.y >= height;
}

void snake_move_ahead(struct Snake *snake)
{
    struct Position nextHeadPosition = snake_next_head_position(snake);
    if (snake->head_index == snake->length - 1) {
        snake->head_index = 0;
    } else {
        snake->head_index++;
    }
    snake->segments[snake->head_index] = nextHeadPosition;
}

void snake_grow(struct Snake *snake)
{
    struct Position nextHeadPosition = snake_next_head_position(snake);
    if (snake->head_index == snake->length) {
        snake->segments[snake->length] = nextHeadPosition;
    } else {
        for (int i = snake->length; i > snake->head_index; i--) {
            snake->segments[i + 1] = snake->segments[i];
        }
        snake->segments[snake->head_index + 1] = nextHeadPosition;
    }
    snake->length++;
}

void paint_background(void)
{
    canvas_set_fill_style(COLOR_BACKGROUND);
    canvas_fill_rect(0, 0, GRID_WIDTH * CELL_SIZE, GRID_HEIGHT * CELL_SIZE);
}

void paint_snake(struct Snake *snake)
{
    canvas_set_fill_style(COLOR_SNAKE);
    for (int i = 0; i < snake->length; i++) {
        struct Position segment = snake->segments[i];
        canvas_fill_rect(segment.x * CELL_SIZE, segment.y * CELL_SIZE, CELL_SIZE, CELL_SIZE);
    }
}

void paint_apple(struct Position apple)
{
    canvas_set_fill_style(COLOR_APPLE);
    canvas_fill_rect(apple.x * CELL_SIZE, apple.y * CELL_SIZE, CELL_SIZE, CELL_SIZE);
}

static struct Snake snake;
static struct Position apple;
static int32_t stepPeriod;
static int32_t score;
static int32_t next_reward;

void change_snake_direction(enum Direction d) 
{
    if (direction_is_opposite(snake.direction, d)) {
        return;
    }
    snake.direction = d;
}

void on_key_down(enum KeyCode code)
{
    switch (code) {
    case KEY_CODE_ARROW_UP:
        change_snake_direction(DIRECTION_UP);
        break;
    case KEY_CODE_ARROW_DOWN:
        change_snake_direction(DIRECTION_DOWN);
        break;
    case KEY_CODE_ARROW_LEFT:
        change_snake_direction(DIRECTION_LEFT);
        break;
    case KEY_CODE_ARROW_RIGHT:
        change_snake_direction(DIRECTION_RIGHT);
        break;
    }
}

void speedup_game(void)
{
    if (stepPeriod > 50) {
        stepPeriod -= 25;
        snake_step_period_updated(stepPeriod);
    }
}

bool snake_will_eat_apple(void) 
{ 
    return position_equals(snake_next_head_position(&snake), apple);
}

void update_score(void)
{
    score += next_reward;
    next_reward += 10;
}

void teleport_apple(void)
{
    apple.x = js_random(GRID_WIDTH);
    apple.y = js_random(GRID_HEIGHT);
}

void repaint(void)
{
    paint_background();
    paint_snake(&snake);
    paint_apple(apple);
    canvas_fill();
}

void step(int32_t timestamp)
{
    if (snake_will_eat_apple()) {
        snake_grow(&snake);
        teleport_apple();
        speedup_game();
        update_score();
        snake_score_changed(score);
    } else {
        snake_move_ahead(&snake);
    }
    if (snake_is_out_of_bounds(&snake, GRID_WIDTH, GRID_HEIGHT) || snake_eats_himself(&snake)) {
        snake_game_over();
    }
    repaint();
}

void init()
{
    stepPeriod = 300;
    next_reward = 10;
    teleport_apple();
    snake.length = 4;
    snake.head_index = 3;
    snake.direction = DIRECTION_RIGHT;
    snake.segments[1].x = 1;
    snake.segments[2].x = 2;
    snake.segments[3].x = 3;
    repaint();
    snake_score_changed(0);
}