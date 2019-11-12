#include <eosiolib/eosio.hpp>

using namespace eosio;

class petelection : public contract
{
private:
  // create the multi index tables to store the data

  /// @abi table
  struct pet {
    uint64_t _key;       // primary key for pet
    std::string _name;   // pet name
    uint32_t _count = 0; // voted count

    uint64_t primary_key() const { return _key; }
  };
  typedef eosio::multi_index<N(pet), pet> pets;

  /// @abi table
  struct voter {
    uint64_t _key;
    uint64_t _pet_key; // name of poll
    account_name _account;   // this account has voted, avoid duplicate voter

    uint64_t primary_key() const { return _key; }
    uint64_t pet_key() const { return _pet_key; }
  };
  typedef eosio::multi_index<N(voter), voter, indexed_by<N(_pet_key), const_mem_fun<voter, uint64_t, &voter::pet_key>>> voters;

  // local instances of the multi indexes
  pets _pets;
  voters _voters;
  uint64_t _pets_count;

public:
  petelection(account_name s) : contract(s), _pets(s, s), _voters(s, s), _pets_count(0) {}

  // public methods exposed via the ABI
  // on pets

  
  void version() {
    print("petelection Smart Contract version 0.0.1\n");
  };

  void addc(std::string name) {
    print("Adding pet ", name, "\n");

    uint64_t key = _pets.available_primary_key();

    // update the table to include a new pet
    _pets.emplace(get_self(), [&](auto &p) {
      p._key = key;
      p._name = name;
      p._count = 0;
    });

    print("pet added successfully. pet_key = ", key, "\n");
  };

  /// @abi action
  void reset() {
    // Get all keys of _pets
    std::vector<uint64_t> keysForDeletion;
    for (auto &itr : _pets) {
      keysForDeletion.push_back(itr.primary_key());
    }

    // now delete each item for that poll
    for (uint64_t key : keysForDeletion) {
      auto itr = _pets.find(key);
      if (itr != _pets.end()) {
        _pets.erase(itr);
      }
    }

    // Get all keys of _voters
    keysForDeletion.empty();
    for (auto &itr : _voters) {
      keysForDeletion.push_back(itr.primary_key());
    }

    // now delete each item for that poll
    for (uint64_t key : keysForDeletion) {
      auto itr = _voters.find(key);
      if (itr != _voters.end()) {
        _voters.erase(itr);
      }
    }

    print("pets and voters reset successfully.\n");
  };

  /// @abi action
  void results() {
    print("Start listing voted results\n");
    for (auto& item : _pets) {
      print("pet ", item._name, " has voted count: ", item._count, "\n");
    }
  };

  /// @abi action
  void vote(account_name s, uint64_t pet_key) {
    require_auth(s);

    bool found = false;

    // Did the voter vote before?
    for (auto& item : _voters) {
      if (item._account == s) {
        found = true;
        break;
      }
    }
    eosio_assert(!found, "You're voted already!");

    // Findout the pet by id
    std::vector<uint64_t> keysForModify;
    for (auto& item : _pets) {
      if (item.primary_key() == pet_key) {
        keysForModify.push_back(item.primary_key());
        break;
      }
    }

    if (keysForModify.size() == 0) {
      eosio_assert(found, "Invalid pet id!");
      return;
    }

    // Update the voted count inside the pet
    for (uint64_t key : keysForModify) {
      auto itr = _pets.find(key);
      auto pet = _pets.get(key);
      if (itr != _pets.end()) {
        _pets.modify(itr, get_self(), [&](auto& p) {
          p._count++;
        });

        print("Voted pet: ", pet._name, " successfully\n");
      }
    }

    // Add this user to voters array
    _voters.emplace(get_self(), [&](auto& p) {
      p._key = _voters.available_primary_key();
      p._pet_key = pet_key;
      p._account = s;
    });
  };
};

EOSIO_ABI(petelection, (version)(reset)(addc)(results)(vote))