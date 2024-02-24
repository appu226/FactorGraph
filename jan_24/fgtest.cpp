#include <dd/bdd_factory.h>
#include <dd/dd.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>


int main();
dd::BddWrapper parseNode(const std::string& expression, const dd::BddVectorWrapper& vars, DdManager* ddm);


int main()
{
    using namespace dd;
    auto ddmp = std::make_unique<dd::ManagerWrapper>(Cudd_Init(0, 0, 256, 262144, 0));
    auto ddm = ddmp->manager;
    BddVectorWrapper vars(ddm);
    const int NumVars = 95;
    for (int i = 1; i <= NumVars; ++i)
        vars.push_back(BddWrapper(bdd_new_var_with_index(ddm, i), ddm));

    std::string node1Str = "-43 OR 39 OR 86 && -39 OR 43 OR 86 && -86 OR -80 OR -43 OR -39 && -86 OR -80 OR 39 OR 43 && -43 OR 39 OR 80 && -39 OR 43 OR 80 && -86 OR -80 OR -39 OR -37 && 37 OR 86 && 37 OR 39 OR 43 && 37 OR 43 OR 80 && 37 OR 41 OR 80 && -43 OR -41 OR -37 && 37 OR 39 OR 41 && -90 OR -43 OR -41 && -43 OR 41 OR 90 && -90 OR 41 OR 43 && -41 OR 43 OR 90 && -91 OR 42 OR 86 && -42 OR 86 OR 91 && -86 OR -79 OR 42 OR 91 && -42 OR 79 OR 91 && -91 OR -86 OR -79 OR -42 && -91 OR 42 OR 79 && -86 OR -79 OR -42 OR 41 && -41 OR 42 && -41 OR 79 && -41 OR 86 && -45 OR -7 OR 37 && -37 OR 7 OR 9 && -86 OR -81 OR -9 OR -7 && 7 OR 86 && -45 OR 9 OR 86 && -9 OR 45 OR 86 && -86 OR -81 OR 9 OR 45 && -9 OR 45 OR 81 && -86 OR -81 OR -45 OR -9 && -45 OR 9 OR 81 && 7 OR 9 OR 45 && 7 OR 45 OR 81 && -37 OR 7 OR 81 && -45 OR -37 OR 89 && 37 OR 45 OR 89 && -89 OR -45 OR 37 && -89 OR -37 OR 45";
    std::string node2Str = "-49 OR -34 OR 10 && -10 OR 35 OR 49 && -35 OR 34 OR 85 && -34 OR 35 OR 85 && -85 OR -82 OR 34 OR 35 && -85 OR -35 OR -34 && -85 OR -34 OR 82 && 34 OR 35 OR 49 && -85 OR -49 OR -35 && 49 OR 85 && -34 OR -10 OR 9 && -34 OR -9 OR 10 && -10 OR -9 OR 34 && 9 OR 10 OR 34 && -87 OR -49 OR -7 && -87 OR -7 OR 82 && -86 OR -82 OR -49 OR 47 && -86 OR -82 OR -47 OR 49 && -49 OR -47 OR 82 && 47 OR 49 OR 82 && -49 OR -47 OR 86 && 47 OR 49 OR 86 && -87 OR 86 && -86 OR -82 OR 49 OR 87 && -87 OR -49 OR 47 && -87 OR 47 OR 82 && -47 OR -7 OR 88 && 7 OR 47 OR 88 && -88 OR -47 OR 7 && -88 OR -7 OR 47 && -47 OR 7 OR 87";
    std::string node3Str = "-28 OR 27 OR 80 && -27 OR 28 OR 80 && -84 OR -80 OR -28 OR -27 OR 79 && -84 OR -80 OR 27 OR 28 OR 79 && -28 OR 27 OR 84 && -27 OR 28 OR 84 && -83 OR -79 OR -27 OR 28 && -79 OR -28 OR 27 && -28 OR 83 && -84 OR -80 OR 27 OR 83 && -94 OR 79 && -83 OR -79 OR 94 && -94 OR 83 && -83 OR 84 OR 85 OR 86 && -85 OR -79 OR -27 OR 26 && -26 OR 27 && -26 OR 79 && -26 OR 85 && -92 OR 27 OR 85 && -85 OR -79 OR 27 OR 92 && -27 OR 79 OR 92 && -27 OR 85 OR 92 && -92 OR -85 OR -79 OR -27 && -92 OR 27 OR 79 && -42 OR -30 OR -26 && -42 OR 26 OR 30 && -30 OR 26 OR 42 && -26 OR 30 OR 42 && -30 OR -26 OR -22 && 22 OR 24 OR 26 && 22 OR 26 OR 80 && -30 OR 24 OR 85 && -24 OR 30 OR 85 && -85 OR -80 OR -30 OR -24 && -85 OR -80 OR 24 OR 30 && -30 OR 24 OR 80 && -24 OR 30 OR 80 && -85 OR -80 OR -24 OR -22 && 22 OR 85 && 22 OR 24 OR 30 && 22 OR 30 OR 80";
    std::string node4Str = "-84 OR -82 OR 12 OR 14 OR 83 && -14 OR 12 OR 84 && -84 OR -82 OR -14 OR -12 && -12 OR 14 OR 82 && -12 OR 14 OR 84 && -84 OR -82 OR 12 OR 14 OR 81 && -14 OR 12 OR 82 && -83 OR -81 OR -12 OR 14 && -84 OR -81 OR -18 OR 83 && 18 OR 81 OR 83 && 18 OR 83 OR 84 && -83 OR -82 OR -18 OR 84 && 18 OR 82 OR 84 && -84 OR -83 OR -82 OR -81 OR 18 && -83 OR -82 OR -18 OR 81 && -84 OR -81 OR -18 OR 82 && 18 OR 81 OR 82 && -84 OR -82 OR -14 OR 35 && -35 OR 82 && -35 OR 84 && -84 OR -82 OR -28 OR 35 && -35 OR 14 OR 28 && -83 OR -81 OR 28 && -28 OR 81 && -18 OR -14 && 14 OR 15 OR 18 && -15 OR -14 && -18 OR 15 OR 24 && -15 OR 18 OR 24 && -24 OR -18 OR -15 && -24 OR 15 OR 18 && -93 OR 79 OR 83 && -83 OR -80 OR 79 OR 93 && -93 OR 83 OR 84 && -84 OR -79 OR 83 OR 93 && -93 OR -84 OR -83 OR -80 OR -79 && -83 OR -80 OR 84 OR 93 && -93 OR 79 OR 80 && -84 OR -79 OR 80 OR 93 && -93 OR 80 OR 84 && -84 OR -83 OR -80 OR -79 OR -15 && -84 OR -83 OR -81 OR -80 OR -15 && 15 OR 80 && 15 OR 83 && 15 OR 84 && 15 OR 79 OR 81 && -79 OR 80 OR 81 OR 82";
    std::string node5Str = "-39 OR -32 OR 22 && -39 OR -22 OR 32 && -32 OR -22 OR 39 && 22 OR 32 OR 39 && -32 OR -10 OR 22 && -22 OR 10 OR 12 && -22 OR 10 OR 81 && -32 OR 12 OR 81 && -12 OR 32 OR 81 && -85 OR -81 OR -32 OR -12 && -85 OR -81 OR 12 OR 32 && -12 OR 32 OR 85 && -32 OR 12 OR 85 && -85 OR -81 OR -12 OR -10 && 10 OR 85 && 10 OR 12 OR 32 && 10 OR 32 OR 81";


    std::string qvStr = "10 && 26 && 84 && 9 && 37 && 22 && 34 && 45 && 80 && 28 && 79 && 14 && 7 && 81 && 15 && 82 && 30 && 86 && 32 && 35 && 12 && 83 && 27 && 42 && 39 && 49 && 41 && 47 && 18 && 43 && 24 && 85";
    // std::string qvStr = "89 && 94 && 87 && 88 && 93 && 90 && 92 && 91";
    BddWrapper qv = parseNode(qvStr, vars, ddm);

    std::vector<std::string> factorStrings = { node1Str, node2Str, node3Str, node4Str, node5Str };
    for (size_t i = 0; i < factorStrings.size(); ++i)
    {
        auto factor = parseNode(factorStrings[i], vars, ddm);
        auto quantified = factor.existentialQuantification(qv);
        std::cout << "Factor " << (i + 1) << " projected " << (quantified.isOne() ? "is ONE" : "is NOT one") << std::endl;
        // bdd_print_minterms(ddm, quantified.getUncountedBdd());
    }

    /*

[DEBUG] ===================== Merged variable nodes =================== 
[DEBUG] 10 && 26 && 84 && 9 && 37 && 22 && 34 && 45 && 80 && 28 && 79 && 14 && 7 && 81 && 15 && 82 && 30 && 86 && 32 && 35 && 12 && 83 && 27 && 42 && 39 && 49 && 41 && 47 && 18 && 43 && 24 && 85
[DEBUG] 89 && 94 && 87 && 88 && 93 && 90 && 92 && 91
*/
    std::cout << "Successful" << std::endl;
    return 0;
}





dd::BddWrapper parseNode(const std::string& expression, const dd::BddVectorWrapper& vars, DdManager* ddm)
{
    std::stringstream ess(expression);
    dd::BddWrapper clause(bdd_zero(ddm), ddm);
    dd::BddWrapper result(bdd_one(ddm), ddm);

    int nextLiteral;
    std::string nextOperator;
    while (ess >> nextLiteral)
    {
        size_t v = std::abs(nextLiteral);
        bool isPositive = (nextLiteral > 0);
        if ( v <= 0 || v > vars->size())
            throw std::runtime_error("Unexpected literal " + std::to_string(nextLiteral) + ", expecting vars between 1 and " + std::to_string(vars->size()));
        clause = clause + (isPositive ? vars.get(v - 1) : -vars.get(v - 1));
        if ( !(ess >> nextOperator) || nextOperator == "&&")
        {
            // std::cout << "Parsed clause\n";
            // bdd_print_minterms(ddm, clause.getUncountedBdd());
            result = result * clause;
            clause = clause.zero();
        }
        else if (nextOperator != "OR")
            throw std::runtime_error("Unpexpected operator \"" + nextOperator + "\", expecting: \"OR\"");
    }

    return result;
}