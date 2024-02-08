#include "iostream"
#include "json.hpp"
#include "httplib.h"
#include "cstdlib"

using json = nlohmann::json;

using namespace std;

const string SERVER_URL = "http://127.0.0.1:5000";

void clearScreen() {
    // Use system-specific command to clear the screen
    #ifdef _WIN32
        system("cls");  // For Windows
    #else
        system("clear");  // For Linux and macOS
    #endif
}

// Get the price information -- Choice No 1
void displaySimulationInformation(const std::string& responseBody) {
    // Parse the JSON data from the response body
    json simulationInfo = json::parse(responseBody);

    // Display the formatted information
    std::cout << "Simulation Information:\n";
    std::cout << "Time: " << simulationInfo["sim_time_hour"] << ":"
              << simulationInfo["sim_time_min"] << "\n";
    std::cout << "Base Current Load: " << simulationInfo["base_current_load"] << "\n";
    std::cout << "Battery Capacity: " << simulationInfo["battery_capacity_kWh"] << " kWh\n";
}

void getSimulationInformation() {
    httplib::Client client(SERVER_URL.c_str());
    auto response = client.Get("/info");

    if (response && response->status == 200) {
        displaySimulationInformation(response->body);
    } else {
        std::cout << "Failed to retrieve simulation information.\n";
    }
}

// Get House Hold information -- Choice No 2
void displayHouseholdEnergyInfo(const std::string& responseBody) {
    try {
        // Parse the JSON data from the response body
        json householdEnergyInfo = json::parse(responseBody);

        // Display the formatted information
        std::cout << "Household Energy Information:\n";

        // Check if the response is an array
        if (householdEnergyInfo.is_array()) {
            // Display energy values with corresponding hours
            for (size_t hour = 1; hour <= householdEnergyInfo.size(); ++hour) {
                std::cout << "Hour " << hour << ": " << householdEnergyInfo[hour - 1] << " kWh\n";
            }
        } else {
            std::cerr << "Invalid structure for household energy entry.\n";
        }
    } catch (const json::exception& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
    }
}

void getHouseholdEnergyInfo() {
    httplib::Client client(SERVER_URL.c_str());
    auto response = client.Get("/baseload");

    if (response && response->status == 200) {
        displayHouseholdEnergyInfo(response->body);
    } else {
        std::cout << "Failed to retrieve household energy information.\n";
    }
}

// Function to display price per hour information -- Choice No 3
void displayPricePerHour(const std::string& responseBody) {
    try {
        // Parse the JSON data from the response body
        json pricePerHour = json::parse(responseBody);

        // Display the formatted information
        std::cout << "Price Per Hour Information:\n";

        // Check if the response is an array
        if (pricePerHour.is_array()) {
            // Display prices with corresponding hours
            for (size_t hour = 1; hour <= pricePerHour.size(); ++hour) {
                std::cout << "Hour " << hour << ": SEK  " << pricePerHour[hour - 1] << "\n";
            }
        } else {
            std::cerr << "Invalid structure for price per hour entry.\n";
        }
    } catch (const json::exception& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
    }
}

void getPricePerHour() {
    httplib::Client client(SERVER_URL.c_str());
    auto response = client.Get("/priceperhour");

    if (response && response->status == 200) {
        displayPricePerHour(response->body);
    } else {
        std::cout << "Failed to retrieve price per hour information.\n";
    }
}

// Start Charging -- Choice No 4
void startCharging() {
    httplib::Client client(SERVER_URL.c_str());
    const char* chargeEndpoint = "/charge";
    const char* batteryEndpoint = "/charge";

    // Send a POST request to start charging
    json payload = {
        {"charging", "on"}
    };

    auto chargeResponse = client.Post(chargeEndpoint, payload.dump(), "application/json");

    if (chargeResponse && chargeResponse->status == 200) {
        cout << "Charging STARTED.\n";
    } else {
        cout << "Failed to start charging.\n";
        return;
    }

    // Continuously check the battery percentage while charging
    while (true) {
        // Get the battery percentage
        auto batteryResponse = client.Get(batteryEndpoint);

        if (batteryResponse && batteryResponse->status == 200) {
            // Remove any newline characters before parsing the battery percentage
            std::string batteryPercentageStr = batteryResponse->body;
            batteryPercentageStr.erase(std::remove(batteryPercentageStr.begin(), batteryPercentageStr.end(), '\n'), batteryPercentageStr.end());

            // Parse the battery percentage as an integer
            int batteryPercentage = std::stoi(batteryPercentageStr);

            // Display the battery percentage
            cout << "Battery Percentage: " << batteryPercentage << "%" << endl;

            // Check if the battery percentage has reached 80%
            if (batteryPercentage >= 80) {
                // Send a POST request to stop charging
                json stopPayload = {
                    {"charging", "off"}
                };

                auto stopChargeResponse = client.Post(chargeEndpoint, stopPayload.dump(), "application/json");

                if (stopChargeResponse && stopChargeResponse->status == 200) {
                    cout << "Charging STOPPED. Battery reached 80%.\n";
                } else {
                    cout << "Failed to stop charging.\n";
                }

                break;
            }
        } else {
            cout << "Failed to retrieve battery percentage.\n";
            break;
        }

        // Add a delay (adjust the duration based on your requirements)
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

// Stop Charging -- Choice No 5
void stopCharging() {
    httplib::Client client(SERVER_URL.c_str());
    const char* chargeEndpoint = "/charge";

    // Send a POST request to stop charging
    json payload = {
        {"charging", "off"}
    };

    auto chargeResponse = client.Post(chargeEndpoint, payload.dump(), "application/json");

    if (chargeResponse && chargeResponse->status == 200) {
        cout << "Charging STOPPED.\n";

        // Get the battery percentage after stopping charging
        auto batteryResponse = client.Get(chargeEndpoint);

        if (batteryResponse && batteryResponse->status == 200) {
            // Remove any newline characters before displaying the battery percentage
            std::string batteryPercentage = batteryResponse->body;
            batteryPercentage.erase(std::remove(batteryPercentage.begin(), batteryPercentage.end(), '\n'), batteryPercentage.end());

            // Display the battery percentage
            cout << "Battery Percentage: " << batteryPercentage << "%" << endl;
        } else {
            cout << "Failed to retrieve battery percentage.\n";
        }
    } else {
        cout << "Failed to stop charging.\n";
    }
}

// DisCharge Battery -- Choice No 6
void disCharge() {
    httplib::Client client(SERVER_URL.c_str());
    const char* dischargeEndpoint = "/discharge";
    const char* batteryEndpoint = "/charge";

    // Send a POST request to start discharging
    json payload = {
        {"discharging", "on"}
    };

    auto dischargeResponse = client.Post(dischargeEndpoint, payload.dump(), "application/json");

    if (dischargeResponse && dischargeResponse->status == 200) {
        cout << "Discharging STARTED.\n";
    } else {
        cout << "Failed to start discharging.\n";
        return;
    }

    // Display the battery percentage during discharging
    while (true) {
        // Get the battery percentage
        auto batteryResponse = client.Get(batteryEndpoint);

        if (batteryResponse && batteryResponse->status == 200) {
            // Remove any newline characters before displaying the battery percentage
            std::string batteryPercentage = batteryResponse->body;
            batteryPercentage.erase(std::remove(batteryPercentage.begin(), batteryPercentage.end(), '\n'), batteryPercentage.end());

            // Display the battery percentage
            cout << "Battery Percentage: " << batteryPercentage << "%" << endl;

            // Check if the battery percentage has reached 20%
            if (std::stoi(batteryPercentage) <= 20) {
                cout << "Battery Percentage reached 20%. Stopping discharging.\n";
                break;
            }
        } else {
            cout << "Failed to retrieve battery percentage.\n";
            break;
        }

        // Add a delay (adjust the duration based on your requirements)
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Send a POST request to stop discharging
    payload = {
        {"discharging", "off"}
    };

    auto stopDischargeResponse = client.Post(dischargeEndpoint, payload.dump(), "application/json");

    if (stopDischargeResponse && stopDischargeResponse->status == 200) {
        cout << "Discharging STOPPED.\n";
    }
}


// Optimize BatteryCharging -- Choice No 7
void optimizeBatteryCharging() {
    // Get household energy information
    httplib::Client client(SERVER_URL.c_str());
    auto responseHousehold = client.Get("/baseload");

    if (!responseHousehold || responseHousehold->status != 200) {
        cout << "Failed to retrieve household energy information.\n";
        return;
    }

    // Get price per hour information
    auto responsePricePerHour = client.Get("/priceperhour");

    if (!responsePricePerHour || responsePricePerHour->status != 200) {
        cout << "Failed to retrieve price per hour information.\n";
        return;
    }

    try {
        // Parse household energy and price per hour JSON data
        json householdEnergyInfo = json::parse(responseHousehold->body);
        json pricePerHour = json::parse(responsePricePerHour->body);

        // Check if both responses are arrays
        if (!householdEnergyInfo.is_array() || !pricePerHour.is_array()) {
            cerr << "Invalid structure for household energy or price per hour entry.\n";
            return;
        }

        // Find the hour with the lowest household consumption
        size_t lowestConsumptionHour = 0;
        double lowestConsumption = numeric_limits<double>::max();

        for (size_t hour = 1; hour <= householdEnergyInfo.size(); ++hour) {
            double consumption = householdEnergyInfo[hour - 1];
            if (consumption < lowestConsumption) {
                lowestConsumption = consumption;
                lowestConsumptionHour = hour;
            }
        }

        // Check if total energy consumption is below 11 kW
        if (lowestConsumption <= 11) {
            cout << "Charging battery when household consumption is lowest (Hour " << lowestConsumptionHour << ").\n";
        } else {
            cout << "Household consumption is too high to charge the battery.\n";
            return;
        }

        // Find the hour with the lowest electricity price
        size_t lowestPriceHour = 0;
        double lowestPrice = numeric_limits<double>::max();

        for (size_t hour = 1; hour <= pricePerHour.size(); ++hour) {
            double price = pricePerHour[hour - 1];
            if (price < lowestPrice) {
                lowestPrice = price;
                lowestPriceHour = hour;
            }
        }
        
        // Check if total energy consumption is below 11 kW
        /*
        if (lowestPrice <= 11) {
            cout << "Charging battery when electricity price is lowest (Hour " << lowestPriceHour << ").\n";
        } else {
            cout << "Total energy consumption is too high to charge the battery.\n";
            return;
        }
        */

        // If you reach here, you can start charging the battery
        startCharging();

    } catch (const json::exception& e) {
        cerr << "Error parsing JSON: " << e.what() << endl;
    }
}


int main() {
    int choice;
    clearScreen();

    do {
        cout << "Menu:" << endl;
        cout << "1. Get Simulation Information" << endl;
        cout << "2. Get Household Energy Information" << endl;
        cout << "3. Get Price Per Hour Information" << endl;
        cout << "4. Start Charging" << endl;
        cout << "5. Stop Charging" << endl;
        cout << "6. DisCharging" << endl;
        cout << "7. Optimize Battery Charging" << endl;
        cout << "0. Exit" << endl;
        cout << "Enter your choice: ";
        cin >> choice;
        cout << "************************************" << endl;

        clearScreen();

        switch (choice) {
            case 1:
                getSimulationInformation();
                cout << "************************************" << endl;
                break;
            case 2:
                getHouseholdEnergyInfo();
                cout << "************************************" << endl;
                break;
            case 3:
                getPricePerHour();
                cout << "************************************" << endl;
                break;
            case 4:
                startCharging();
                cout << "************************************" << endl;
                break;
            case 5:
                stopCharging();
                cout << "************************************" << endl;
                break;
            case 6:
                disCharge();
                cout << "************************************" << endl;
                break;
            case 7:
                optimizeBatteryCharging();
                cout << "************************************" << endl;
                break;
            case 0:
                cout << "Exiting the application. Goodbye!" << endl;
                cout << "************************************" << endl;
                break;
            default:
                cout << "Invalid choice. Please try again." << endl;
                cout << "************************************" << endl;
        }

    } while (choice != 0);

    return 0;
}
