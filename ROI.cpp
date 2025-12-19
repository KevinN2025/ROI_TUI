#include <ncurses.h>
#include <cstdlib>
#include <cerrno>

// ================= Financial Logic =================

float netprofit(float sell_price, float cost)
{
    return sell_price - cost;
}

float roi(float net_profit, float cost)
{
    if (cost == 0.0f)
        return 0.0f;

    return (net_profit / cost) * 100;
}

// ================= TUI Application =================

class ROITuiApp {
private:
    float sell_price;
    float cost;
    float net_profit;
    float ROI;

    int selected_field; // 0 = sell, 1 = cost, 2 = calculate

    bool input_error;
    char error_msg[80];

public:
    ROITuiApp()
        : sell_price(0), cost(0), net_profit(0), ROI(0),
          selected_field(0), input_error(false)
    {
        error_msg[0] = '\0';
    }

    void init()
    {
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);

        start_color();
        init_pair(1, COLOR_WHITE, COLOR_BLACK); // normal
        init_pair(2, COLOR_BLACK, COLOR_CYAN);  // selected
        init_pair(3, COLOR_GREEN, COLOR_BLACK); // profit
        init_pair(4, COLOR_RED, COLOR_BLACK);   // loss
        init_pair(5, COLOR_YELLOW, COLOR_BLACK);// error
    }

    void shutdown()
    {
        endwin();
    }

    // ---------- Error handling ----------

    void setError(const char* msg)
    {
        input_error = true;
        snprintf(error_msg, sizeof(error_msg), "%s", msg);
    }

    void clearError()
    {
        input_error = false;
        error_msg[0] = '\0';
    }

    // ---------- Parsing ----------

    bool parseFloat(const char* str, float& out)
    {
        errno = 0;
        char* end = nullptr;
        float val = strtof(str, &end);

        if (errno != 0 || end == str || *end != '\0')
            return false;

        out = val;
        return true;
    }

    // ---------- UI ----------

    void draw()
    {
        clear();

        mvprintw(1, 2, "ROI Calculator (ncurses TUI)");
        mvprintw(2, 2, "TAB/arrows navigate | ENTER edit | Q quit");

        drawField(4, "Sell Price", sell_price, 0);
        drawField(5, "Cost", cost, 1);

        if (selected_field == 2)
            attron(COLOR_PAIR(2));
        mvprintw(7, 2, "[ Calculate ]");
        if (selected_field == 2)
            attroff(COLOR_PAIR(2));

        // Compute only if no input error
        if (!input_error && cost != 0.0f) {
            net_profit = netprofit(sell_price, cost);
            ROI = roi(net_profit, cost);
        }

        if (net_profit >= 0)
            attron(COLOR_PAIR(3));
        else
            attron(COLOR_PAIR(4));

        mvprintw(9, 2, "Net Profit: %.2f", net_profit);
        mvprintw(10, 2, "ROI: %.2f%%", ROI);

        attroff(COLOR_PAIR(3));
        attroff(COLOR_PAIR(4));

        // Error/status line
        if (input_error) {
            attron(COLOR_PAIR(5));
            mvprintw(12, 2, "Error: %s", error_msg);
            attroff(COLOR_PAIR(5));
        }

        refresh();
    }

    void drawField(int y, const char* label, float value, int index)
    {
        if (selected_field == index)
            attron(COLOR_PAIR(2));

        mvprintw(y, 2, "%s: %.2f", label, value);

        if (selected_field == index)
            attroff(COLOR_PAIR(2));
    }

    void editField(float& field)
    {
        char buffer[32];

        echo();
        curs_set(1);
        clearError();

        mvprintw(14, 2, "Enter value: ");
        getnstr(buffer, sizeof(buffer) - 1);

        noecho();
        curs_set(0);

        float value;
        if (!parseFloat(buffer, value)) {
            setError("Invalid: Please enter a number");
            return;
        }

        if (value < 0.0f) {
            setError("Value cannot be negative");
            return;
        }

        field = value;
    }

    // ---------- Event loop ----------

    void run()
    {
        int ch;
        bool running = true;

        while (running) {
            draw();
            ch = getch();

            switch (ch) {
            case 'q':
            case 'Q':
                running = false;
                break;

            case KEY_UP:
            case KEY_LEFT:
                selected_field = (selected_field + 2) % 3;
                break;

            case KEY_DOWN:
            case KEY_RIGHT:
            case '\t':
                selected_field = (selected_field + 1) % 3;
                break;

            case '\n':
                if (selected_field == 0)
                    editField(sell_price);
                else if (selected_field == 1)
                    editField(cost);
                else if (selected_field == 2 && cost == 0.0f)
                    setError("Cost cannot be zero");
                break;
            }
        }
    }
};

// ================= main =================

int main()
{
    ROITuiApp app;
    app.init();
    app.run();
    app.shutdown();
    return 0;
}

