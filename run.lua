function run(pet, pet1)
    print(lua_engine)
    print("nihao")
    print(type(pet))
    print(pet:GetAge())
    pet:SetAge(30)
    print(pet:GetAge())
    Pet.GetAge = function(self)
        return 10086
    end
    print("1", pet1:GetAge())
end